#pragma once

namespace juce
{
    class ComponentPeer;

    // Sends a key event to the host view holding an embedded JUCE view (as the AU wrapper does in Live), then gives
    // the embedded view its window's keys back if the host's view still has them.
    void forwardKeyToHostView (void* embeddedNSView, void* nsKeyEvent);

    // The peer holding JUCE's keyboard focus on macOS, or nullptr: for tests of the focus bookkeeping.
    ComponentPeer* getFocusedNSViewPeer() noexcept;
}
