// hello_go: the program. Its interface is main.xml, loaded when it starts:
// save main.xml while it runs and the window follows. F5 builds and runs it.
package main

import (
	"runtime"

	"raven/raven"
)

func main() {
	// Raven belongs to the thread that starts it
	runtime.LockOSThread()
	if raven.Init(raven.R_BACKEND_VULKAN) == 0 {
		return
	}
	raven.LocaleLoadCsv("strings.csv")
	raven.LocaleSetLanguage(raven.LocaleGetSystemLanguage())
	window := raven.WindowCreateFromXmlFile("main.xml")
	clicks := 0
	// OnClicked="hello.clicked" in main.xml fires this action
	raven.ActionBind("hello.clicked", func(sender raven.ID, action string) {
		clicks++
		// the widget with Name="status"
		raven.LabelSetText(raven.WidgetFind(window, "status"), raven.TrPlural("clicks", clicks))
	})
	for raven.DrawFrame() != 0 {
	}
	raven.Shutdown()
}
