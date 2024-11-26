#include "nexusFile.h"
#include "processors.h"
#include "window.h"
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <optional>

namespace Processors
{
void dumpEventTimesEpoch(const std::vector<std::string> &inputNeXusFiles, int detectorIndex, bool toStdOut)
{
    /*
     * Get all events for the specified detector spectrum, returning seconds since epoch for each
     */

    fmt::print("Retrieving all events from detector index {}...\n", detectorIndex);

    // Loop over input NeXuS files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the NeXuS file ready for use
        NeXuSFile nxs(nxsFileName);
        nxs.prepareSpectraSpace();
        nxs.loadEventData();

        std::optional<double> lastSecondsSinceEpoch;
        auto eventStart = 0, eventEnd = 0;
        const auto &eventsPerFrame = nxs.eventsPerFrame();
        const auto &eventIndices = nxs.eventIndices();
        const auto &eventTimes = nxs.eventTimes();
        const auto &frameOffsets = nxs.frameOffsets();
        const auto spectrumId = nxs.spectrumForDetector(detectorIndex);
        fmt::print("NeXuS file spectrum ID for detector index {} is {}.\n", detectorIndex, spectrumId);

        std::ofstream fileOutput;

        if (!toStdOut)
            fileOutput.open(fmt::format("{}.events.{}", nxsFileName, detectorIndex).c_str());

        std::ostream &output = toStdOut ? std::cout : fileOutput;
        output << fmt::format("# {:20s}  {:20s}  {:20s}  {}\n", "frame_offset(us)", "start_time_offset(s)", "epoch_offset(s)",
                              "delta(s)");

        // Loop over frames in the NeXuS file
        for (auto frameIndex = 0; frameIndex < nxs.eventsPerFrame().size(); ++frameIndex)
        {
            // Set new end event index and get zero for frame
            eventEnd += eventsPerFrame[frameIndex];
            auto frameZero = frameOffsets[frameIndex];

            for (auto k = eventStart; k < eventEnd; ++k)
            {
                if (eventIndices[k] == spectrumId)
                {
                    auto eMicroSeconds = eventTimes[k];
                    auto eSeconds = eMicroSeconds * 0.000001;
                    auto eSecondsSinceEpoch = eSeconds + frameZero + nxs.startSinceEpoch();
                    if (lastSecondsSinceEpoch)
                        output << fmt::format("{:20.6f}  {:20.10f}  {:20.5f}  {}\n", eMicroSeconds, eSeconds + frameZero,
                                              eSecondsSinceEpoch, eSecondsSinceEpoch - *lastSecondsSinceEpoch);
                    else
                        output << fmt::format("{:20.6f}  {:20.10f}  {:20.5f}\n", eMicroSeconds, eSeconds + frameZero,
                                              eSecondsSinceEpoch);

                    lastSecondsSinceEpoch = eSecondsSinceEpoch;
                }
            }

            // Update start event index
            eventStart = eventEnd;
        }

        if (!toStdOut)
            fileOutput.close();
    }
}

} // namespace Processors
