#include "nexusFile.h"
#include "processors.h"
#include "window.h"
#include <fmt/core.h>
#include <stdexcept>

namespace Processors
{
// Partition events into summed windows / slices
void partitionEventsSummed(const std::vector<std::string> &inputNeXusFiles, std::string_view outputFilePath,
                           const Window &windowDefinition, int nSlices, double windowDelta)
{
    /*
     * From our main windowDefinition we will continually propagate it forwards in time (by the window delta) splitting it into
     * nSlices and until we go over the end time of the current file.
     */

    fmt::print("Partitioning events in summed windows/slices...\n");

    // Generate a new set of window "slices" and associated output NeXuS files to sum data into
    auto slices = prepareSlices(windowDefinition, nSlices, inputNeXusFiles[0], outputFilePath);

    // Initialise the slice iterator and window slice / NeXuSFile references
    auto sliceIt = slices.begin();

    // Loop over input NeXuS files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the NeXuS file and get its event data
        NeXuSFile nxs(nxsFileName, true);
        nxs.loadEventData();

        const auto &eventsPerFrame = nxs.eventsPerFrame();
        const auto &eventIndices = nxs.eventIndices();
        const auto &eventTimes = nxs.eventTimes();
        const auto &frameOffsets = nxs.frameOffsets();

        // Loop over frames in the NeXuS file
        auto eventStart = 0, eventEnd = 0;
        for (auto frameIndex = 0; frameIndex < nxs.eventsPerFrame().size(); ++frameIndex)
        {
            // Set new end event index and get zero for frame
            eventEnd += eventsPerFrame[frameIndex];
            auto frameZero = frameOffsets[frameIndex] + nxs.startSinceEpoch();

            // If the current slice end time is less than the frame zero, iterate the slices.
            while (sliceIt->first.endTime() < frameZero)
            {
                sliceIt++;

                // If we have run out of slices propagate the set forward.
                if (sliceIt == slices.end())
                {
                    sliceIt = slices.begin();
                    for (auto &&[slice, _unused] : slices)
                        slice.shiftStartTime(windowDelta);
                    printf("Propagated window forwards... new start time is %16.2f\n", sliceIt->first.startTime());
                }
            }

            // If this frame zero is greater than or equal to the start time of the current window slice we can process events
            if (frameZero >= sliceIt->first.startTime())
            {
                // Sanity check!
                if (frameZero > sliceIt->first.endTime())
                    throw(std::runtime_error("Somebody's done something wrong here....\n"));

                // Grab the destination datafile for this slice and bin events
                auto &destinationHistograms = sliceIt->second.detectorHistograms();
                for (int k = eventStart; k < eventEnd; ++k)
                {
                    auto id = eventIndices[k];
                    if (id > 0)
                        gsl_histogram_accumulate(destinationHistograms[id], eventTimes[k], 1.0);
                }

                // Increment the frame counter for this slice
                sliceIt->second.incrementDetectorFrameCount();
            }

            // Update start event index
            eventStart = eventEnd;
        }
    }

    // Perform post-processing
    postProcess(slices);

    // Save slices
    saveSlices(slices);
}

} // namespace Processors
