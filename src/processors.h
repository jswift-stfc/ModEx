#pragma once

#include <string>
#include <vector>

// Forward Declarations
class NeXuSFile;
class Window;

namespace Processors
{
// Available Processing Modes
enum class ProcessingMode
{
    None,
    DumpHistogram,
    DumpEvents,
    PartitionEventsIndividual,
    PartitionEventsSummed
};

// Processing Direction
enum class ProcessingDirection
{
    Forwards
};
// Processing direction
extern Processors::ProcessingDirection processingDirection_;

// Available Post-Processing Types
enum class PostProcessingMode
{
    None,
    ScaleDetectors,
    ScaleMonitors
};
// Selected post-processing mode
extern Processors::PostProcessingMode postProcessingMode_;

/*
 * Common Functions
 */

// Prepare slices for specified Window
std::vector<std::pair<Window, NeXuSFile>> prepareSlices(const Window &window, int nSlices, std::string templatingSourceFilename,
                                                        std::string_view outputFilePath);
// Perform any post-processing required
void postProcess(std::vector<std::pair<Window, NeXuSFile>> &slices);
// Write slice data
void saveSlices(std::vector<std::pair<Window, NeXuSFile>> &slices);

/*
 * Processors
 */

// Get Events
std::map<int, std::vector<double>> dumpEvents(const std::vector<std::string> &inputNeXusFiles, int detectorId,
                                              bool firstOnly = false);
// Dump histogram from specified spectrum
void dumpHistogram(const std::vector<std::string> &inputNeXusFiles, int spectrumId, bool firstOnly = false);
// Partition events into individual windows / slices
void partitionEventsIndividual(const std::vector<std::string> &inputNeXusFiles, std::string_view outputFilePath,
                               const Window &windowDefinition, int nSlices, double windowDelta);
// Partition events into summed windows / slices
void partitionEventsSummed(const std::vector<std::string> &inputNeXusFiles, std::string_view outputFilePath,
                           const Window &windowDefinition, int nSlices, double windowDelta);
}; // namespace Processors
