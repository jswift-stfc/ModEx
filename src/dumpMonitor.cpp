#include "nexusFile.h"
#include "processors.h"
#include <fmt/core.h>
#include <fstream>

namespace Processors
{
void dumpMonitor(const std::vector<std::string> &inputNeXusFiles, int monitorIndex)
{
    /*
     * Dump histograms for the specified monitor index
     */

    fmt::print("Dumping histogram for monitor index {}...\n", monitorIndex);

    // Loop over input NeXuS files
    for (auto &nxsFileName : inputNeXusFiles)
    {
        // Open the NeXuS file and load in monitor counts
        NeXuSFile nxs(nxsFileName);
        nxs.prepareSpectraSpace();
        nxs.loadMonitorCounts();

        // Open the output file
        std::ofstream output(fmt::format("{}.mon.{}", nxsFileName, monitorIndex).c_str());
        output << "# TCB/us   Counts\n";
        auto bin = 0;
        const auto &counts = nxs.monitorCounts().at(monitorIndex);
        for (auto tof : nxs.tofBoundaries())
        {
            output << fmt::format("{}  {}\n", tof, counts[bin++]);
        }
        output.close();
    }
}

} // namespace Processors
