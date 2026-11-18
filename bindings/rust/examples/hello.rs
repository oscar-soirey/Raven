// A window described in XML, driven from Rust.
//
//   RAVEN_LIB_DIR=/path/to/lib cargo run --example hello
//
// `-- --test` runs without a window system and without a GPU, clicks its own
// button and checks what happened: it is how the build of Raven checks this
// binding.

use raven::*;
use std::cell::RefCell;
use std::rc::Rc;

const XML: &str = r#"
<Window Title="Raven from Rust" Width="360" Height="200">
    <VBox Spacing="10" Slot.Padding="16">
        <Label Name="title" Text="Hello from Rust" TextSize="20"/>
        <TextInput Name="name" Placeholder="Your name"/>
        <HBox Spacing="8">
            <Button Name="greet" Text="Greet" Style="Primary" OnClicked="greet" Slot.Fill="1"/>
            <Button Text="Quit" OnClicked="quit"/>
        </HBox>
    </VBox>
</Window>"#;

fn main() {
    let test = std::env::args().any(|argument| argument == "--test");
    unsafe {
        let mut desc: RInitDesc = std::mem::zeroed();
        desc.backend = if test { R_BACKEND_NULL } else { R_BACKEND_DEFAULT };
        desc.headless = test as i32;
        if Rinit_ex(&desc) == 0 {
            eprintln!("cannot start Raven");
            std::process::exit(1);
        }

        let window = window_from_xml(XML);
        let title = widget_find(window, "title");
        let name = widget_find(window, "name");
        let clicks = Rc::new(RefCell::new(0));

        let counted = clicks.clone();
        action_bind("greet", move |_sender, _action| {
            *counted.borrow_mut() += 1;
            let who = text(Rtext_input_get_text(name));
            Rlabel_set_text(title, cstr(&format!("Hello, {}!", who)).as_ptr());
        });
        action_bind("quit", |_, _| Rquit(0));

        if !test {
            while Rwait_frame(-1.0) != 0 {}
            Rshutdown();
            return;
        }

        Rdraw_frame();
        assert_eq!(text(Rwindow_get_title(window)), "Raven from Rust");
        assert_eq!(Rwidget_get_type(title), R_WIDGET_LABEL);
        Rtext_input_set_text(name, cstr("Zoé").as_ptr());
        let (mut x, mut y, mut width, mut height) = (0.0f32, 0.0f32, 0.0f32, 0.0f32);
        Rwidget_get_rect(widget_find(window, "greet"), &mut x, &mut y, &mut width, &mut height);
        assert!(width > 0.0 && height > 0.0);
        Rwindow_inject_mouse_move(window, x + width / 2.0, y + height / 2.0);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 1);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 0);
        assert_eq!(*clicks.borrow(), 1);
        assert_eq!(text(Rlabel_get_text(title)), "Hello, Zoé!");

        // structures, and a mistake described in one
        let mut stats: RMemoryStats = std::mem::zeroed();
        Rmemory_get_stats(&mut stats);
        assert!(stats.allocation_count > 0);
        let mut error: RXmlError = std::mem::zeroed();
        let broken = "<A><B></A>";
        assert_eq!(Rxml_document_parse(broken.as_ptr() as *const _, broken.len(), &mut error), R_INVALID_ID);
        assert_eq!(error.line, 1);
        assert!(text(error.message.as_ptr()).contains("does not match"));

        // documents
        let document = Rxml_document_create(cstr("VBox").as_ptr());
        let root = Rxml_document_get_root(document);
        let label = Rxml_node_add_child(root, cstr("Label").as_ptr(), -1);
        Rxml_node_set_attribute(label, cstr("Text").as_ptr(), cstr("a < b").as_ptr());
        assert!(text(Rxml_document_to_string(document)).contains("Text=\"a &lt; b\""));
        let built = Rxml_document_create_widget(document);
        assert_eq!(Rwidget_get_xml_node(Rwidget_get_child(built, 0)), label);
        Rwidget_destroy(built);
        Rxml_document_destroy(document);

        Raction_fire(cstr("quit").as_ptr(), R_INVALID_ID);
        assert_eq!(Ris_running(), 0);
        Rshutdown();
    }
    println!("rust binding: ok");
}
