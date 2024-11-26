#include "nexusFile.h"
#include "processors.h"
#include "window.h"
#include <fmt/core.h>
#include <iomanip>
#include <sstream>

// Externals
Processors::ProcessingDirection Processors::processingDirection_ = Processors::ProcessingDirection::Forwards;
Processors::PostProcessingMode Processors::postProcessingMode_ = Processors::PostProcessingMode::None;

namespace Processors
{
// Prepare slices for specified Window
std::vector<std::pair<Window, NeXuSFile>> prepareSlices(const Window &window, int nSlices, std::string templatingSourceFilename,
                                                        std::string_view outputFilePath)
{
    std::vector<std::pair<Window, NeXuSFile>> slices;
    const auto sliceDuration = window.duration() / nSlices;
    auto sliceStartTime = window.startTime();

    // Output slice details on first prep
    static bool firstRun = true;
    if (firstRun)
    {
        fmt::print("Input window duration was {} seconds.\n", window.duration());
        fmt::print("Number of slices into which window will be partitioned is {}.\n", nSlices);
        fmt::print("Individual slice duration is {} seconds.\n", sliceDuration);
    }

    // Open the source NeXuS file
    NeXuSFile sourceNxs(templatingSourceFilename);

    for (auto i = 0; i < nSlices; ++i)
    {
        std::stringstream outputFileName;
        outputFileName << outputFilePath << window.id() << "-" << std::to_string(int(window.startTime()));
        if (nSlices > 1)
            outputFileName << "-" << std::setw(3) << std::setfill('0') << (i + 1);
        outputFileName << ".nxs";

        std::stringstream sliceName;
        sliceName << window.id() << i + 1;

        // Template the NeXuS file
        NeXuSFile::templateTo(templatingSourceFilename, outputFileName.str());

        auto &[newWin, nexus] =
            slices.emplace_back(Window(sliceName.str(), sliceStartTime, sliceDuration), NeXuSFile(outputFileName.str()));
        nexus.loadBasicData();
        nexus.prepareSpectraSpace();
        nexus.loadMonitorCounts();

        sliceStartTime += sliceDuration;
    }

    return slices;
}

// Perform any post-processing required
void postProcess(std::vector<std::pair<Window, NeXuSFile>> &slices)
{
    double factor = 0.0;
    for (auto &&[slice, outputNeXuSFile] : slices)
    {
        fmt::print("Output '{}' ({} -> {}) has {} detector frames and {} monitor frames.\n", std::string(slice.id()).c_str(),
                   slice.startTime(), slice.endTime(), outputNeXuSFile.nDetectorFrames(), outputNeXuSFile.nMonitorFrames());
        switch (postProcessingMode_)
        {
            case (Processors::PostProcessingMode::None):
                break;
            case (Processors::PostProcessingMode::ScaleMonitors):
                factor = (double)outputNeXuSFile.nDetectorFrames() / (double)outputNeXuSFile.nMonitorFrames();
                fmt::print(" --> Scaling monitors by processed detector-to-monitor frame ratio ({}).\n", factor);
                outputNeXuSFile.scaleMonitors(factor);
                break;
            case (Processors::PostProcessingMode::ScaleDetectors):
                factor = (double)outputNeXuSFile.nMonitorFrames() / (double)outputNeXuSFile.nDetectorFrames();
                fmt::print(" --> Scaling detectors by processed monitor-to-detector frame ratio ({}).\n", factor);
                outputNeXuSFile.scaleDetectors(factor);
                break;
            default:
                throw(std::runtime_error("Unhandled post processing mode.\n"));
        }
    }
}

// Write slice data
void saveSlices(std::vector<std::pair<Window, NeXuSFile>> &slices)
{
    for (auto &&[slice, outputNeXuSFile] : slices)
    {
        printf("Writing data to output NeXuS file '%s' for slice '%s'...\n", outputNeXuSFile.filename().c_str(),
               std::string(slice.id()).c_str());

        if (!outputNeXuSFile.saveModifiedData())
            fmt::print("!! Error saving file '{}'.", outputNeXuSFile.filename());
    }
}
} // namespace Processors
