/**
 * @file main.cpp
 */
/* Copyright (C) 2022-2026 by Arjan van Vught mailto:info@gd32-dmx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma GCC push_options
#pragma GCC optimize("O2")
#pragma GCC optimize("no-tree-loop-distribute-patterns")

#include "board.h"
#include "watchdog.h"
#include "network.h"
#include "displayudf.h"
#include "json/displayudfparams.h"
#include "dmxnodenode.h"
#include "dmxnodemsgconst.h"
#ifdef ARTNET_HAVE_TRIGGER
#include "artnettriggerhandler.h"
#endif // ARTNET_HAVE_TRIGGER
#include "firmware/pixeldmx/show.h"
#include "pixeltestpattern.h"
#include "json/pixeldmxparams.h"
#include "pixeldmxmulti.h"
#ifdef NODE_SHOWFILE
#include "showfile.h"
#endif // NODE_SHOWFILE
#include "remoteconfig.h"
#include "configstore.h"
#include "firmware/firmwareversion.h"
#include "software_version.h"

namespace board {
void RebootHandler() {
    PixelDmxMulti::Get().Blackout();
    ArtNetNode::Get()->Stop();
}
} // namespace board

int main() { // NOLINT
    board::Init();
    DisplayUdf display;
    ConfigStore config_store;
    network::Init();
    FirmwareVersion fw(kSoftwareVersion, __DATE__, __TIME__);

    fw.Print("Art-Net 4 Pixel controller {" STR(CONFIG_DMXNODE_PIXEL_MAX_PORTS) " Ports}");

    DmxNodeNode dmxnode_node;
    PixelDmxMulti pixeldmx_multi;
    PixelTestPattern pixeltest_pattern(pixelpatterns::Pattern::kNone, CONFIG_DMXNODE_PIXEL_MAX_PORTS);

    json::PixelDmxParams pixeldmx_params;
    pixeldmx_params.Load();
    pixeldmx_params.Set();

    const auto kPixelActivePorts = pixeldmx_multi.GetOutputPorts();
    const auto kTestPattern = pixeltest_pattern.GetPattern();

    if (kTestPattern != pixelpatterns::Pattern::kNone) {
        dmxnode_node.SetOutput(nullptr);
    } else {
        dmxnode_node.SetOutput(&pixeldmx_multi);
    }

#ifdef ARTNET_HAVE_TRIGGER
    ArtNetTriggerHandler trigger_handler(&pixeldmx_multi);
#endif // ARTNET_HAVE_TRIGGER

#ifdef NODE_SHOWFILE
    ShowFile showfile;
    showfile.Print();
#endif // NODE_SHOWFILE

    dmxnode_node.Print();
    pixeldmx_multi.Print();

    display.SetTitle("ArtNet 4 Pixel %dx%d", kPixelActivePorts, pixeldmx_multi.GetCount());
    display.Set(2, displayudf::Labels::kVersion);
    display.Set(3, displayudf::Labels::kIp);
    display.Set(4, displayudf::Labels::kHostname);
    display.Set(5, displayudf::Labels::kBoardname);

    json::DisplayUdfParams displayudf_params;
    displayudf_params.Load();
    displayudf_params.SetAndShow();

    common::firmware::pixeldmx::Show(7, kTestPattern);

    RemoteConfig remote_config(remoteconfig::Output::PIXEL, kPixelActivePorts);

    display.TextStatus(DmxNodeMsgConst::kStart, ansi::Colours::Colour::kYellow);

    dmxnode_node.Start();

    display.TextStatus(DmxNodeMsgConst::kStarted, ansi::Colours::Colour::kGreen);

    watchdog::Init();

    for (;;) {
        watchdog::Feed();
        network::Run();
        dmxnode_node.Run();
#ifdef NODE_SHOWFILE
        showfile.Run();
#endif // NODE_SHOWFILE
        pixeltest_pattern.Run();
        board::Run();
    }
}
