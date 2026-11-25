// Raven on iOS from Swift: the UIKit entry of ../ios/src/main.m written in
// Swift, with the Swift binding (bindings/swift) over Raven.xcframework.
// `import Raven` is the C API of the xcframework and the helpers of the binding
// (actionBind, windowFromXml, text...); build.sh compiles Raven.swift as the
// Swift part of the xcframework's module. See ../README.md, "Swift".

import UIKit
import Raven

let layout = """
<Window Title="Raven from Swift">
    <VBox Spacing="16" Slot.Padding="24">
        <Label Text="Raven from Swift" TextSize="28"/>
        <Label Name="count" Text="Not tapped yet" TextSize="18"/>
        <Button Name="tap" Text="Tap me" Style="Primary" OnClicked="tap"/>
        <Label Name="version" Opacity="0.6" Wrap="true"/>
    </VBox>
</Window>
"""

@main
final class AppDelegate: UIResponder, UIApplicationDelegate {
    func application(_ application: UIApplication,
                     didFinishLaunchingWithOptions options: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        return true
    }
}

// (named in Info.plist: UISceneDelegateClassName)
@objc(RavenSceneDelegate)
final class SceneDelegate: UIResponder, UIWindowSceneDelegate {
    // Raven makes its own UIWindow on the scene
    var window: UIWindow?
    private var link: CADisplayLink?
    private var taps = 0

    func scene(_ scene: UIScene, willConnectTo session: UISceneSession, options: UIScene.ConnectionOptions) {
        errorSetCallback { code, severity, detail in
            print("[raven] error \(code.rawValue) severity \(severity.rawValue): \(detail)")
        }
        var desc = RInitDesc()
        // R_BACKEND_DEFAULT: Metal on iOS
        desc.backend = R_BACKEND_DEFAULT
        // (a text for as long as Raven runs)
        desc.app_name = UnsafePointer(strdup("Raven from Swift"))
        guard Rinit_ex(&desc) != 0 else {
            print("[raven] Rinit failed")
            return
        }
        Rtheme_follow_system(1)

        let window = windowFromXml(layout)
        let count = Rwidget_find(window, "count")
        Rlabel_set_text(Rwidget_find(window, "version"), "Raven \(text(Rget_version())), Swift, \(UIDevice.current.systemName) \(UIDevice.current.systemVersion)")
        actionBind("tap") { [weak self] _, _ in
            guard let self = self else { return }
            self.taps += 1
            Rlabel_set_text(count, "Tapped \(self.taps) time\(self.taps == 1 ? "" : "s")")
        }

        // UIKit owns the run loop: the display link calls Rwait_frame, which
        // pauses it while Raven has nothing to draw
        let link = CADisplayLink(target: self, selector: #selector(tick(_:)))
        link.add(to: .main, forMode: .common)
        rvn_uikit_set_display_link(link)
        self.link = link

        tapForTesting(Rwidget_find(window, "tap"), count)
    }

    @objc private func tick(_ link: CADisplayLink) {
        if Rwait_frame(-1) == 0 {
            link.invalidate()
            print("[raven] Rwait_frame returned false")
        }
    }

    // Testing hook: RAVEN_TAPS=<n> (SIMCTL_CHILD_RAVEN_TAPS from simctl) taps
    // the button n times, from 2 s after the launch, with Rtest_tap (a touch
    // given to Raven as the platform gives it).
    private func tapForTesting(_ button: Rid, _ label: Rid) {
        guard let value = ProcessInfo.processInfo.environment["RAVEN_TAPS"], let n = Int(value), n > 0 else { return }
        for i in 0..<n {
            DispatchQueue.main.asyncAfter(deadline: .now() + 2.0 + 0.5 * Double(i)) {
                let done = Rtest_tap(button) != 0
                // (called from outside a frame, a change asks for no frame by itself
                // on iOS: the display link stays paused until Rwake_up)
                Rwake_up()
                print("[raven] test tap \(i + 1): \(done ? "done" : "refused"), the label says \"\(text(Rlabel_get_text(label)))\"")
            }
        }
    }
}
