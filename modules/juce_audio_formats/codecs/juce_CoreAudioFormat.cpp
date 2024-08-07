/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#if JUCE_MAC || JUCE_IOS

#include <juce_audio_basics/native/juce_CoreAudioLayouts_mac.h>
#include <juce_core/native/juce_CFHelpers_mac.h>

#import <AVFoundation/AVFoundation.h>

namespace juce
{

//==============================================================================
namespace
{
    const char* const coreAudioFormatName = "CoreAudio supported file";

    StringArray getStringInfo (AudioFilePropertyID property, UInt32 size, void* data)
    {
        CFObjectHolder<CFArrayRef> extensions;
        UInt32 sizeOfArray = sizeof (extensions.object);

        const auto err = AudioFileGetGlobalInfo (property,
                                                 size,
                                                 data,
                                                 &sizeOfArray,
                                                 &extensions.object);

        if (err != noErr)
            return {};

        const auto numValues = CFArrayGetCount (extensions.object);

        StringArray extensionsArray;

        for (CFIndex i = 0; i < numValues; ++i)
            extensionsArray.add ("." + String::fromCFString ((CFStringRef) CFArrayGetValueAtIndex (extensions.object, i)));

        return extensionsArray;
    }

    StringArray findFileExtensionsForCoreAudioCodec (AudioFileTypeID type)
    {
        return getStringInfo (kAudioFileGlobalInfo_ExtensionsForType, sizeof (AudioFileTypeID), &type);
    }

    StringArray findFileExtensionsForCoreAudioCodecs [[maybe_unused]]()
    {
        return getStringInfo (kAudioFileGlobalInfo_AllExtensions, 0, nullptr);
    }

    static AudioFileTypeID toAudioFileTypeID (CoreAudioFormat::StreamKind kind)
    {
        using StreamKind = CoreAudioFormat::StreamKind;

        switch (kind)
        {
            case StreamKind::kAiff:                 return kAudioFileAIFFType;
            case StreamKind::kAifc:                 return kAudioFileAIFCType;
            case StreamKind::kWave:                 return kAudioFileWAVEType;
            case StreamKind::kSoundDesigner2:       return kAudioFileSoundDesigner2Type;
            case StreamKind::kNext:                 return kAudioFileNextType;
            case StreamKind::kMp3:                  return kAudioFileMP3Type;
            case StreamKind::kMp2:                  return kAudioFileMP2Type;
            case StreamKind::kMp1:                  return kAudioFileMP1Type;
            case StreamKind::kAc3:                  return kAudioFileAC3Type;
            case StreamKind::kAacAdts:              return kAudioFileAAC_ADTSType;
            case StreamKind::kMpeg4:                return kAudioFileMPEG4Type;
            case StreamKind::kM4a:                  return kAudioFileM4AType;
            case StreamKind::kM4b:                  return kAudioFileM4BType;
            case StreamKind::kCaf:                  return kAudioFileCAFType;
            case StreamKind::k3gp:                  return kAudioFile3GPType;
            case StreamKind::k3gp2:                 return kAudioFile3GP2Type;
            case StreamKind::kAmr:                  return kAudioFileAMRType;

            case StreamKind::kNone:                 break;
        }

        return {};
    }
    constexpr auto DefaultBitsPerSample = 32;
    constexpr auto InitialReusuableBufferSize = 2048;
}

//==============================================================================
const char* const CoreAudioFormat::midiDataBase64   = "midiDataBase64";
const char* const CoreAudioFormat::tempo            = "tempo";
const char* const CoreAudioFormat::timeSig          = "time signature";
const char* const CoreAudioFormat::keySig           = "key signature";

//==============================================================================
struct CoreAudioFormatMetatdata
{
    static uint32 chunkName (const char* const name) noexcept   { return ByteOrder::bigEndianInt (name); }

    //==============================================================================
    struct FileHeader
    {
        FileHeader (InputStream& input)
        {
            fileType    = (uint32) input.readIntBigEndian();
            fileVersion = (uint16) input.readShortBigEndian();
            fileFlags   = (uint16) input.readShortBigEndian();
        }

        uint32 fileType;
        uint16 fileVersion;
        uint16 fileFlags;
    };

    //==============================================================================
    struct ChunkHeader
    {
        ChunkHeader (InputStream& input)
        {
            chunkType = (uint32) input.readIntBigEndian();
            chunkSize = (int64)  input.readInt64BigEndian();
        }

        uint32 chunkType;
        int64 chunkSize;
    };

    //==============================================================================
    struct AudioDescriptionChunk
    {
        AudioDescriptionChunk (InputStream& input)
        {
            sampleRate          = input.readDoubleBigEndian();
            formatID            = (uint32) input.readIntBigEndian();
            formatFlags         = (uint32) input.readIntBigEndian();
            bytesPerPacket      = (uint32) input.readIntBigEndian();
            framesPerPacket     = (uint32) input.readIntBigEndian();
            channelsPerFrame    = (uint32) input.readIntBigEndian();
            bitsPerChannel      = (uint32) input.readIntBigEndian();
        }

        double sampleRate;
        uint32 formatID;
        uint32 formatFlags;
        uint32 bytesPerPacket;
        uint32 framesPerPacket;
        uint32 channelsPerFrame;
        uint32 bitsPerChannel;
    };

    //==============================================================================
    static StringPairArray parseUserDefinedChunk (InputStream& input, int64 size)
    {
        StringPairArray infoStrings;
        auto originalPosition = input.getPosition();

        uint8 uuid[16];
        input.read (uuid, sizeof (uuid));

        if (memcmp (uuid, "\x29\x81\x92\x73\xB5\xBF\x4A\xEF\xB7\x8D\x62\xD1\xEF\x90\xBB\x2C", 16) == 0)
        {
            auto numEntries = (uint32) input.readIntBigEndian();

            for (uint32 i = 0; i < numEntries && input.getPosition() < originalPosition + size; ++i)
            {
                String keyName = input.readString();
                infoStrings.set (keyName, input.readString());
            }
        }

        input.setPosition (originalPosition + size);
        return infoStrings;
    }

    //==============================================================================
    static StringPairArray parseMidiChunk (InputStream& input, int64 size)
    {
        auto originalPosition = input.getPosition();

        MemoryBlock midiBlock;
        input.readIntoMemoryBlock (midiBlock, (ssize_t) size);
        MemoryInputStream midiInputStream (midiBlock, false);

        StringPairArray midiMetadata;
        MidiFile midiFile;

        if (midiFile.readFrom (midiInputStream))
        {
            midiMetadata.set (CoreAudioFormat::midiDataBase64, midiBlock.toBase64Encoding());

            findTempoEvents (midiFile, midiMetadata);
            findTimeSigEvents (midiFile, midiMetadata);
            findKeySigEvents (midiFile, midiMetadata);
        }

        input.setPosition (originalPosition + size);
        return midiMetadata;
    }

    static void findTempoEvents (MidiFile& midiFile, StringPairArray& midiMetadata)
    {
        MidiMessageSequence tempoEvents;
        midiFile.findAllTempoEvents (tempoEvents);

        auto numTempoEvents = tempoEvents.getNumEvents();
        MemoryOutputStream tempoSequence;

        for (int i = 0; i < numTempoEvents; ++i)
        {
            auto tempo = getTempoFromTempoMetaEvent (tempoEvents.getEventPointer (i));

            if (tempo > 0.0)
            {
                if (i == 0)
                    midiMetadata.set (CoreAudioFormat::tempo, String (tempo));

                if (numTempoEvents > 1)
                    tempoSequence << String (tempo) << ',' << tempoEvents.getEventTime (i) << ';';
            }
        }

        if (tempoSequence.getDataSize() > 0)
            midiMetadata.set ("tempo sequence", tempoSequence.toUTF8());
    }

    static double getTempoFromTempoMetaEvent (MidiMessageSequence::MidiEventHolder* holder)
    {
        if (holder != nullptr)
        {
            auto& midiMessage = holder->message;

            if (midiMessage.isTempoMetaEvent())
            {
                auto tempoSecondsPerQuarterNote = midiMessage.getTempoSecondsPerQuarterNote();

                if (tempoSecondsPerQuarterNote > 0.0)
                    return 60.0 / tempoSecondsPerQuarterNote;
            }
        }

        return 0.0;
    }

    static void findTimeSigEvents (MidiFile& midiFile, StringPairArray& midiMetadata)
    {
        MidiMessageSequence timeSigEvents;
        midiFile.findAllTimeSigEvents (timeSigEvents);
        auto numTimeSigEvents = timeSigEvents.getNumEvents();

        MemoryOutputStream timeSigSequence;

        for (int i = 0; i < numTimeSigEvents; ++i)
        {
            int numerator, denominator;
            timeSigEvents.getEventPointer(i)->message.getTimeSignatureInfo (numerator, denominator);

            String timeSigString;
            timeSigString << numerator << '/' << denominator;

            if (i == 0)
                midiMetadata.set (CoreAudioFormat::timeSig, timeSigString);

            if (numTimeSigEvents > 1)
                timeSigSequence << timeSigString << ',' << timeSigEvents.getEventTime (i) << ';';
        }

        if (timeSigSequence.getDataSize() > 0)
            midiMetadata.set ("time signature sequence", timeSigSequence.toUTF8());
    }

    static void findKeySigEvents (MidiFile& midiFile, StringPairArray& midiMetadata)
    {
        MidiMessageSequence keySigEvents;
        midiFile.findAllKeySigEvents (keySigEvents);
        auto numKeySigEvents = keySigEvents.getNumEvents();

        MemoryOutputStream keySigSequence;

        for (int i = 0; i < numKeySigEvents; ++i)
        {
            auto& message (keySigEvents.getEventPointer (i)->message);
            auto key = jlimit (0, 14, message.getKeySignatureNumberOfSharpsOrFlats() + 7);
            bool isMajor = message.isKeySignatureMajorKey();

            static const char* majorKeys[] = { "Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#" };
            static const char* minorKeys[] = { "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#", "G#", "D#", "A#" };

            String keySigString (isMajor ? majorKeys[key]
                                         : minorKeys[key]);

            if (! isMajor)
                keySigString << 'm';

            if (i == 0)
                midiMetadata.set (CoreAudioFormat::keySig, keySigString);

            if (numKeySigEvents > 1)
                keySigSequence << keySigString << ',' << keySigEvents.getEventTime (i) << ';';
        }

        if (keySigSequence.getDataSize() > 0)
            midiMetadata.set ("key signature sequence", keySigSequence.toUTF8());
    }

    //==============================================================================
    static StringPairArray parseInformationChunk (InputStream& input)
    {
        StringPairArray infoStrings;
        auto numEntries = (uint32) input.readIntBigEndian();

        for (uint32 i = 0; i < numEntries; ++i)
            infoStrings.set (input.readString(), input.readString());

        return infoStrings;
    }

    //==============================================================================
    static bool read (InputStream& input, StringPairArray& metadataValues)
    {
        auto originalPos = input.getPosition();

        const FileHeader cafFileHeader (input);
        const bool isCafFile = cafFileHeader.fileType == chunkName ("caff");

        if (isCafFile)
        {
            while (! input.isExhausted())
            {
                const ChunkHeader chunkHeader (input);

                if (chunkHeader.chunkType == chunkName ("desc"))
                {
                    AudioDescriptionChunk audioDescriptionChunk (input);
                }
                else if (chunkHeader.chunkType == chunkName ("uuid"))
                {
                    metadataValues.addArray (parseUserDefinedChunk (input, chunkHeader.chunkSize));
                }
                else if (chunkHeader.chunkType == chunkName ("data"))
                {
                    // -1 signifies an unknown data size so the data has to be at the
                    // end of the file so we must have finished the header

                    if (chunkHeader.chunkSize == -1)
                        break;

                    input.setPosition (input.getPosition() + chunkHeader.chunkSize);
                }
                else if (chunkHeader.chunkType == chunkName ("midi"))
                {
                    metadataValues.addArray (parseMidiChunk (input, chunkHeader.chunkSize));
                }
                else if (chunkHeader.chunkType == chunkName ("info"))
                {
                    metadataValues.addArray (parseInformationChunk (input));
                }
                else
                {
                    // we aren't decoding this chunk yet so just skip over it
                    input.setPosition (input.getPosition() + chunkHeader.chunkSize);
                }
            }
        }

        input.setPosition (originalPos);

        return isCafFile;
    }
};

class CoreAudioReader : public juce::AudioFormatReader
{
public:
    using StreamKind = juce::CoreAudioFormat::StreamKind;

    CoreAudioReader(juce::InputStream* sourceStream, StreamKind streamKind) :
        AudioFormatReader(sourceStream, coreAudioFormatName),
        ok(false),
        audioFile(nil),
        reusableBuffer(nil)
    {
        // Convert to FileInputStream and get the path
        auto fileSourceStream = dynamic_cast<juce::FileInputStream*>(sourceStream);

        if (fileSourceStream == nullptr)
        {
            NSLog(@"Error casting to FileInputStream");
            return;
        }

        const auto pathName = fileSourceStream->getFile().getFullPathName();

        // Assuming sourceStream points to a valid file path
        NSString *filePath = [NSString stringWithUTF8String:pathName.toRawUTF8()];
        NSURL* fileURL = [NSURL fileURLWithPath:filePath];

        NSError* error = nil;
        audioFile = [[AVAudioFile alloc] initForReading:fileURL commonFormat:AVAudioPCMFormatFloat32 interleaved:NO error:&error];
        if (error)
        {
            NSLog(@"Error opening audio file: %@", error);
            return;
        }

        bitsPerSample = DefaultBitsPerSample;
        sampleRate = audioFile.fileFormat.sampleRate;
        numChannels = audioFile.fileFormat.channelCount;
        lengthInSamples = static_cast<juce::int64>(audioFile.length);
        usesFloatingPointData = true;

        const auto channelLayout = audioFile.fileFormat.channelLayout.layout;
        createChannelMap(channelLayout);

        reusableBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:audioFile.processingFormat frameCapacity:InitialReusuableBufferSize];
        
        ok = true;
    }

    ~CoreAudioReader() = default;

    bool readSamples (int* const* destSamples, int numDestChannels, int startOffsetInDestBuffer,
                      juce::int64 startSampleInFile, int numSamples) override
    {
        clearSamplesBeyondAvailableLength (destSamples, numDestChannels, startOffsetInDestBuffer,
                                           startSampleInFile, numSamples, lengthInSamples);

        if (audioFile == nil)
        {
            NSLog(@"Audio file is nil");
            return false;
        }

        if (!reusableBuffer ||
            ![reusableBuffer.format isEqual:audioFile.processingFormat] ||
            reusableBuffer.frameCapacity < numSamples)
        {
            NSLog(@"Creating new reusable buffer");
            reusableBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:audioFile.processingFormat frameCapacity:numSamples];
        }

        // Now check if startSampleInFile is the same as the current file position, if not change it
        if (audioFile.framePosition != startSampleInFile)
        {
            audioFile.framePosition = startSampleInFile;
        }

        NSError* error = nil;
        [audioFile readIntoBuffer:reusableBuffer frameCount:numSamples error:&error];
        if (error)
        {
            NSLog(@"Error reading audio file: %@", error);
            return false;
        }

        const auto numBytes = static_cast<size_t>(numSamples * sizeof(float));

        // Also mostly copied from previous JUCE Reader
        for (int i = numDestChannels; --i >= 0;)
        {
            auto* dest = destSamples[(i < static_cast<int>(numChannels) ? channelMap[i] : i)];

            if (dest != nullptr)
            {
                if (i < static_cast<int>(numChannels))
                {
                    std::memcpy(dest + startOffsetInDestBuffer, reusableBuffer.floatChannelData[i], numBytes);
                }
                else
                {
                    zeromem(dest + startOffsetInDestBuffer, numBytes);
                }
            }
        }

        return true;
    }
    
private:
    // Mostly copied from all the existing juce stuff, left relatively untouched (hence why it looks messy)
    void createChannelMap(const AudioChannelLayout* channelLayout)
    {
        channelMap.malloc(numChannels);
        if (channelLayout != nullptr)
        {
            auto fileLayout = juce::CoreAudioLayouts::fromCoreAudio(*channelLayout);
            AudioChannelSet channelSet;

            if (fileLayout.size() == static_cast<int>(numChannels))
            {
                channelSet = fileLayout;
            }
            
            const auto caOrder = juce::CoreAudioLayouts::getCoreAudioLayoutChannels(*channelLayout);
            for (int i = 0; i < static_cast<int>(numChannels); ++i)
            {
                auto idx = channelSet.getChannelIndexForType(caOrder.getReference(i));
                jassert(isPositiveAndBelow (idx, static_cast<int>(numChannels)));
                channelMap[i] = idx;
            }
        }
        else
        {
            for (int i = 0; i < static_cast<int>(numChannels); ++i)
            {
                channelMap[i] = i;
            }
        }
    }

public:
    bool ok;
    
private:
    AVAudioFile* audioFile;
    AVAudioPCMBuffer* reusableBuffer;
    HeapBlock<int> channelMap;
 
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CoreAudioReader)
};

static AudioFormatID formatForFileType (AudioFileTypeID fileType)
{
    AudioFormatID formatIds[10];
    UInt32 sizeOfArray = sizeof (formatIds);
    AudioFileGetGlobalInfo (
        kAudioFileGlobalInfo_AvailableFormatIDs, sizeof (fileType), (void*) &fileType, &sizeOfArray, &formatIds);
    jassert (sizeOfArray != 0);
    return formatIds[0];
}

static void fillAudioStreamBasicDescription (AudioStreamBasicDescription* fmt)
{
    UInt32 sz = sizeof (AudioStreamBasicDescription);
    OSStatus e [[maybe_unused]] = AudioFormatGetProperty (kAudioFormatProperty_FormatInfo, 0, nullptr, &sz, fmt);
    jassertquiet (e == noErr);
}

class CoreAudioWriter : public AudioFormatWriter
{
public:
    CoreAudioWriter (
        OutputStream* out, AudioFileTypeID fileType, double sr, unsigned int numberOfChannels, unsigned int bitsPerSamp)
    : AudioFormatWriter (out, coreAudioFormatName, sr, numberOfChannels, bitsPerSamp)
    {
        usesFloatingPointData = true;
        {
            AudioStreamBasicDescription fmt;
            memset (&fmt, 0, sizeof (fmt));
            fmt.mSampleRate = sr;
            fmt.mChannelsPerFrame = numberOfChannels;
            fmt.mFormatID = formatForFileType (fileType);
            OSStatus e [[maybe_unused]] = AudioFileInitializeWithCallbacks (
                this,
                &readCallback,
                &writeCallback,
                &getSizeCallback,
                &setSizeCallback,
                fileType,
                &fmt,
                0,
                &audioFileID);
            jassertquiet (e == noErr);
        }
        ExtAudioFileWrapAudioFileID (audioFileID, true, &audioFileRef);
        {
            AudioStreamBasicDescription fmt;
            memset (&fmt, 0, sizeof (fmt));
            fmt.mSampleRate = sr;
            fmt.mChannelsPerFrame = numberOfChannels;
            fmt.mFormatID = kAudioFormatLinearPCM;
            fmt.mFormatFlags =
                kLinearPCMFormatFlagIsFloat | kLinearPCMFormatFlagIsNonInterleaved | kAudioFormatFlagsNativeEndian;
            fmt.mBitsPerChannel = sizeof (float) * 8;
            fmt.mBytesPerFrame = sizeof (float);
            fillAudioStreamBasicDescription (&fmt);
            OSStatus e [[maybe_unused]] =
                ExtAudioFileSetProperty (audioFileRef, kExtAudioFileProperty_ClientDataFormat, sizeof (fmt), &fmt);
            jassertquiet (e == noErr);
        }
        bufferList.malloc (1, sizeof (AudioBufferList) + numChannels * sizeof (::AudioBuffer));
        bufferList->mNumberBuffers = numChannels;
        srcPos = 0;
    }

    ~CoreAudioWriter() override
    {
        ExtAudioFileDispose (audioFileRef);
        AudioFileClose (audioFileID);
    }

    bool write (const int** samplesToWrite, int numSamples) override
    {
        for (int j = (int) numChannels; --j >= 0;)
        {
            bufferList->mBuffers[j].mNumberChannels = 1;
            bufferList->mBuffers[j].mDataByteSize = (UInt32) numSamples * sizeof (float);
            bufferList->mBuffers[j].mData = (void*) samplesToWrite[j];
        }
        return ExtAudioFileWrite (audioFileRef, (UInt32) numSamples, bufferList) == noErr;
    }

    bool flush() override
    {
        output->flush();
        return true;
    }

    SInt64 size = 0;

private:
    AudioFileID audioFileID;
    ExtAudioFileRef audioFileRef;
    HeapBlock<AudioBufferList> bufferList;
    SInt64 srcPos;

    static OSStatus writeCallback (
        void* inClientData, SInt64 inPosition, UInt32 requestCount, const void* buffer, UInt32* actualCount)
    {
        auto* self = static_cast<CoreAudioWriter*> (inClientData);
        self->output->setPosition (inPosition);
        if (! self->output->write (buffer, requestCount))
        {
            jassertfalse;
            return -1;
        }
        *actualCount = requestCount;
        self->size += requestCount;
        return noErr;
    }

    static OSStatus
    readCallback (void* inClientData, SInt64 inPosition, UInt32 requestCount, void* buffer, UInt32* actualCount)
    {
        auto* self = static_cast<CoreAudioWriter*> (inClientData);

        // For formats that require the read callback,
        // CoreAudioWriter supports it for specific output stream types: FileOutputStream and MemoryOutputStream.
        // These are the built-in JUCE output streams that conceptually support reading.
        // A more robust solution would have been if JUCE OutputStreams had the option (not implemented by all) to also
        // support reads.

        auto* file = dynamic_cast<FileOutputStream*> (self->output);
        if (file != nullptr)
        {
            FileInputStream in (file->getFile());
            jassert (in.openedOk());
            {
                bool setPositionOK [[maybe_unused]] = in.setPosition (inPosition);
                jassertquiet (setPositionOK);
            }
            *actualCount = (UInt32) in.read (buffer, (int) requestCount);
            return noErr;
        }

        auto* mem = dynamic_cast<MemoryOutputStream*> (self->output);
        if (mem != nullptr)
        {
            const int remain = jmax (0, (int) mem->getDataSize() - (int) inPosition);
            const size_t count = (size_t) jmin (requestCount, (UInt32) remain);
            *actualCount = (UInt32) count;
            memcpy (buffer, (char*) mem->getData() + inPosition, count);
            return noErr;
        }

        return -1;
    }
    static SInt64 getSizeCallback (void* inClientData)
    {
        auto* self = static_cast<CoreAudioWriter*> (inClientData);
        return self->size;
    }
    static OSStatus setSizeCallback (void* inClientData, SInt64 size)
    {
        auto* self = static_cast<CoreAudioWriter*> (inClientData);
        if (self->size == size)
            return noErr;

        if (auto* out = dynamic_cast<FileOutputStream*> (self->output))
        {
            bool setPositionOK [[maybe_unused]] = out->setPosition (size);
            jassertquiet (setPositionOK);
            Result truncatedOK [[maybe_unused]] = out->truncate();
            jassertquiet (truncatedOK);
        }
        return noErr;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CoreAudioWriter)
};

//==============================================================================
CoreAudioFormat::CoreAudioFormat (StreamKind kind)
: AudioFormat (
    String (coreAudioFormatName) + " " + String ((int) kind),
    findFileExtensionsForCoreAudioCodec (toAudioFileTypeID (kind)))
, streamKind (kind)
{
}

CoreAudioFormat::~CoreAudioFormat() = default;

Array<int> CoreAudioFormat::getPossibleSampleRates()    { return {}; }
Array<int> CoreAudioFormat::getPossibleBitDepths()      { return {}; }

bool CoreAudioFormat::canDoStereo()     { return true; }
bool CoreAudioFormat::canDoMono()       { return true; }

//==============================================================================
AudioFormatReader* CoreAudioFormat::createReaderFor (InputStream* sourceStream,
                                                     bool deleteStreamIfOpeningFails)
{
    std::unique_ptr<CoreAudioReader> r (new CoreAudioReader (sourceStream, streamKind));

    if (r->ok)
        return r.release();

    if (! deleteStreamIfOpeningFails)
        r->input = nullptr;


    return nullptr;
}

AudioFormatWriter* CoreAudioFormat::createWriterFor (
    OutputStream* output,
    double sampleRateToUse,
    unsigned int numberOfChannels,
    int bitsPerSample,
    const StringPairArray& /*metadataValues*/,
    int /*qualityOptionIndex*/)
{
    return new CoreAudioWriter (output, toAudioFileTypeID (streamKind), sampleRateToUse, numberOfChannels, (unsigned int) bitsPerSample);
}

void CoreAudioFormat::registerFormats (AudioFormatManager& formats)
{
    for (int k = (int) StreamKind::kAiff; k <= (int) StreamKind::kAmr; ++k)
        formats.registerFormat (new CoreAudioFormat ((StreamKind) k), false);
}

//==============================================================================
//==============================================================================
#if JUCE_UNIT_TESTS

#define DEFINE_CHANNEL_LAYOUT_DFL_ENTRY(x) CoreAudioChannelLayoutTag { x, #x, AudioChannelSet() }
#define DEFINE_CHANNEL_LAYOUT_TAG_ENTRY(x, y) CoreAudioChannelLayoutTag { x, #x, y }

class CoreAudioLayoutsUnitTest  : public UnitTest
{
public:
    CoreAudioLayoutsUnitTest()
        : UnitTest ("Core Audio Layout <-> JUCE channel layout conversion", UnitTestCategories::audio)
    {}

    // some ambisonic tags which are not explicitly defined
    enum
    {
        kAudioChannelLayoutTag_HOA_ACN_SN3D_0Order = (190U<<16) | 1,
        kAudioChannelLayoutTag_HOA_ACN_SN3D_1Order = (190U<<16) | 4,
        kAudioChannelLayoutTag_HOA_ACN_SN3D_2Order = (190U<<16) | 9,
        kAudioChannelLayoutTag_HOA_ACN_SN3D_3Order = (190U<<16) | 16,
        kAudioChannelLayoutTag_HOA_ACN_SN3D_4Order = (190U<<16) | 25,
        kAudioChannelLayoutTag_HOA_ACN_SN3D_5Order = (190U<<16) | 36
    };

    void runTest() override
    {
        auto& knownTags = getAllKnownLayoutTags();

        {
            // Check that all known tags defined in CoreAudio SDK version 10.12.4 are known to JUCE
            // Include all defined tags even if there are duplicates as Apple will sometimes change
            // definitions
            beginTest ("All CA tags handled");

            for (auto tagEntry : knownTags)
            {
                auto labels = CoreAudioLayouts::fromCoreAudio (tagEntry.tag);

                expect (! labels.isDiscreteLayout(), "Tag \"" + String (tagEntry.name) + "\" is not handled by JUCE");
            }
        }

        {
            beginTest ("Number of speakers");

            for (auto tagEntry : knownTags)
            {
                auto labels = CoreAudioLayouts::getSpeakerLayoutForCoreAudioTag (tagEntry.tag);

                expect (labels.size() == (tagEntry.tag & 0xffff), "Tag \"" + String (tagEntry.name) + "\" has incorrect channel count");
            }
        }

        {
            beginTest ("No duplicate speaker");

            for (auto tagEntry : knownTags)
            {
                auto labels = CoreAudioLayouts::getSpeakerLayoutForCoreAudioTag (tagEntry.tag);
                labels.sort();

                for (int i = 0; i < (labels.size() - 1); ++i)
                    expect (labels.getReference (i) != labels.getReference (i + 1),
                            "Tag \"" + String (tagEntry.name) + "\" has the same speaker twice");
            }
        }

        {
            beginTest ("CA speaker list and juce layouts are consistent");

            for (auto tagEntry : knownTags)
                expect (AudioChannelSet::channelSetWithChannels (CoreAudioLayouts::getSpeakerLayoutForCoreAudioTag (tagEntry.tag))
                            == CoreAudioLayouts::fromCoreAudio (tagEntry.tag),
                        "Tag \"" + String (tagEntry.name) + "\" is not converted consistently by JUCE");
        }

        {
            beginTest ("AudioChannelSet documentation is correct");

            for (auto tagEntry : knownTags)
            {
                if (tagEntry.equivalentChannelSet.isDisabled())
                    continue;

                expect (CoreAudioLayouts::fromCoreAudio (tagEntry.tag) == tagEntry.equivalentChannelSet,
                        "Documentation for tag \"" + String (tagEntry.name) + "\" is incorrect");
            }
        }

        {
            beginTest ("CA tag reverse conversion");

            for (auto tagEntry : knownTags)
            {
                if (tagEntry.equivalentChannelSet.isDisabled())
                    continue;

                expect (CoreAudioLayouts::toCoreAudio (tagEntry.equivalentChannelSet) == tagEntry.tag,
                        "Incorrect reverse conversion for tag \"" + String (tagEntry.name) + "\"");
            }
        }
    }

private:
    struct CoreAudioChannelLayoutTag
    {
        AudioChannelLayoutTag tag;
        const char* name;
        AudioChannelSet equivalentChannelSet; /* referred to this in the AudioChannelSet documentation */
    };

    //==============================================================================
    const Array<CoreAudioChannelLayoutTag>& getAllKnownLayoutTags() const
    {
        static CoreAudioChannelLayoutTag tags[] = {
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_Mono,   AudioChannelSet::mono()),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_Stereo, AudioChannelSet::stereo()),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_StereoHeadphones),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_MatrixStereo),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_MidSide),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_XY),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_Binaural),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_Ambisonic_B_Format),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_Quadraphonic, AudioChannelSet::quadraphonic()),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_Pentagonal, AudioChannelSet::pentagonal()),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_Hexagonal, AudioChannelSet::hexagonal()),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_Octagonal, AudioChannelSet::octagonal()),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_Cube),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_MPEG_1_0),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_MPEG_2_0),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_MPEG_3_0_A, AudioChannelSet::createLCR()),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_MPEG_3_0_B),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_MPEG_4_0_A, AudioChannelSet::createLCRS()),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_MPEG_4_0_B),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_MPEG_5_0_A, AudioChannelSet::create5point0()),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_MPEG_5_0_B),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_MPEG_5_0_C),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_MPEG_5_0_D),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_MPEG_5_1_A, AudioChannelSet::create5point1()),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_MPEG_5_1_B),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_MPEG_5_1_C),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_MPEG_5_1_D),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_MPEG_6_1_A, AudioChannelSet::create6point1()),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_MPEG_7_1_A, AudioChannelSet::create7point1SDDS()),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_MPEG_7_1_B),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_MPEG_7_1_C, AudioChannelSet::create7point1()),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_Emagic_Default_7_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_SMPTE_DTV),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_ITU_1_0),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_ITU_2_0),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_ITU_2_1, AudioChannelSet::createLRS()),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_ITU_2_2),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_ITU_3_0),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_ITU_3_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_ITU_3_2),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_ITU_3_2_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_ITU_3_4_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_0),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_2),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_3),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_4),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_5),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_6),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_7),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_8),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_9),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_10),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_11),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_12),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_13),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_14),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_15),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_16),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_17),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_18),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_19),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DVD_20),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AudioUnit_4),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AudioUnit_5),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AudioUnit_6),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AudioUnit_8),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AudioUnit_5_0),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_AudioUnit_6_0, AudioChannelSet::create6point0()),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_AudioUnit_7_0, AudioChannelSet::create7point0()),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_AudioUnit_7_0_Front, AudioChannelSet::create7point0SDDS()),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AudioUnit_5_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AudioUnit_6_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AudioUnit_7_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AudioUnit_7_1_Front),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AAC_3_0),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AAC_Quadraphonic),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AAC_4_0),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AAC_5_0),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AAC_5_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AAC_6_0),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AAC_6_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AAC_7_0),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AAC_7_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AAC_7_1_B),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AAC_7_1_C),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AAC_Octagonal),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_TMH_10_2_std),
            // DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_TMH_10_2_full), no indication on how to handle this tag
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AC3_1_0_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AC3_3_0),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AC3_3_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AC3_3_0_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AC3_2_1_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_AC3_3_1_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_EAC_6_0_A),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_EAC_7_0_A),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_EAC3_6_1_A),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_EAC3_6_1_B),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_EAC3_6_1_C),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_EAC3_7_1_A),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_EAC3_7_1_B),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_EAC3_7_1_C),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_EAC3_7_1_D),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_EAC3_7_1_E),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_EAC3_7_1_F),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_EAC3_7_1_G),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_EAC3_7_1_H),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DTS_3_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DTS_4_1),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_DTS_6_0_A, AudioChannelSet::create6point0Music()),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DTS_6_0_B),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DTS_6_0_C),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_DTS_6_1_A, AudioChannelSet::create6point1Music()),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DTS_6_1_B),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DTS_6_1_C),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DTS_7_0),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DTS_7_1),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DTS_8_0_A),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DTS_8_0_B),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DTS_8_1_A),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DTS_8_1_B),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DTS_6_1_D),
            DEFINE_CHANNEL_LAYOUT_DFL_ENTRY (kAudioChannelLayoutTag_DTS_6_1_D),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_HOA_ACN_SN3D_0Order,  AudioChannelSet::ambisonic (0)),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_HOA_ACN_SN3D_1Order,  AudioChannelSet::ambisonic (1)),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_HOA_ACN_SN3D_2Order,  AudioChannelSet::ambisonic (2)),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_HOA_ACN_SN3D_3Order, AudioChannelSet::ambisonic (3)),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_HOA_ACN_SN3D_4Order, AudioChannelSet::ambisonic (4)),
            DEFINE_CHANNEL_LAYOUT_TAG_ENTRY (kAudioChannelLayoutTag_HOA_ACN_SN3D_5Order, AudioChannelSet::ambisonic (5))
        };
        static Array<CoreAudioChannelLayoutTag> knownTags (tags, sizeof (tags) / sizeof (CoreAudioChannelLayoutTag));

        return knownTags;
    }
};

static CoreAudioLayoutsUnitTest coreAudioLayoutsUnitTest;

#endif

} // namespace juce

#endif
