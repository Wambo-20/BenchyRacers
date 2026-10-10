extends RefCounted
class_name GraphicsSettings

const DEFAULTS={"display":1,"resolution":0,"quality":2,"vsync":true,"fps":0,"msaa":2,"shadows":true,"reflections":true,"ambient":true,"bloom":true,"dof":true}
const RESOLUTIONS=[Vector2i.ZERO,Vector2i(1280,720),Vector2i(1920,1080),Vector2i(2560,1440),Vector2i(3840,2160)]
const FPS=[0,60,120,144,240]
var game
var values:Dictionary=DEFAULTS.duplicate()
var draft:Dictionary={}
var overlay:Control
var widgets:Dictionary={}
var save_path:String="user://settings.cfg"

func setup(owner_node)->void:
    game=owner_node
    for key in DEFAULTS:values[key]=game.config.get_value("graphics",key,DEFAULTS[key])
    for key in ["display","resolution","quality","fps","msaa"]:
        values[key]=clampi(int(values[key]),0,{"display":1,"resolution":4,"quality":3,"fps":4,"msaa":3}[key])
    for key in ["vsync","shadows","reflections","ambient","bloom","dof"]:values[key]=bool(values[key])

func apply_display()->void:
    Engine.max_fps=FPS[values.fps]
    if DisplayServer.get_name()=="headless":return
    if "--test" in OS.get_cmdline_user_args() and "--test-display" not in OS.get_cmdline_user_args():
        DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
    if "--test" not in OS.get_cmdline_user_args() or "--test-display" in OS.get_cmdline_user_args():
        DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN if values.display==1 else DisplayServer.WINDOW_MODE_WINDOWED)
        if values.display==0:
            var size=RESOLUTIONS[values.resolution]
            var available=DisplayServer.screen_get_usable_rect(DisplayServer.window_get_current_screen())
            if size==Vector2i.ZERO:size=Vector2i(1280,800)
            size=Vector2i(mini(size.x,available.size.x),mini(size.y,available.size.y))
            DisplayServer.window_set_size(size)
            DisplayServer.window_set_position(available.position+(available.size-size)/2)
    DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_ENABLED if values.vsync else DisplayServer.VSYNC_DISABLED)
    _render_resolution()

func _render_resolution()->void:
    var scale=1.0
    if values.resolution>0:
        var width=DisplayServer.window_get_size().x if DisplayServer.get_name()!="headless" else 1440
        scale=clampf(float(RESOLUTIONS[values.resolution].x)/maxf(1,width),0.25,2.0)
    game.get_viewport().scaling_3d_scale=scale

func apply_world()->void:
    var viewport=game.get_viewport()
    viewport.msaa_3d=[Viewport.MSAA_DISABLED,Viewport.MSAA_2X,Viewport.MSAA_4X,Viewport.MSAA_8X][values.msaa]
    _render_resolution()
    var lens=game.camera.attributes as CameraAttributesPractical
    if lens:lens.dof_blur_far_enabled=values.dof
    if not is_instance_valid(game.world):return
    var env=game.world.environment.environment
    env.ssr_enabled=values.reflections and game.world.map_id!=0
    env.ssao_enabled=values.ambient
    env.ssil_enabled=values.ambient and values.quality>=2
    env.glow_enabled=values.bloom
    env.ssr_max_steps=[16,32,48,80][values.quality]
    for child in game.world.get_children():
        if child is DirectionalLight3D:
            child.shadow_enabled=values.shadows
            child.directional_shadow_max_distance=[65,100,160,220][values.quality]
    var plane=PlaneMesh.new();plane.size=Vector2(800,800)
    plane.subdivide_width=[320,480,800,1000][values.quality];plane.subdivide_depth=plane.subdivide_width
    game.world.water.mesh=plane
    if game.world.map_id==0:
        game.world.water.material_override.set_shader_parameter("reflections",values.reflections)
        game.world.water.material_override.set_shader_parameter("reflection_steps",[8,16,24,40][values.quality])

func store()->void:
    for key in values:game.config.set_value("graphics",key,values[key])
    game.config.save(save_path)

func close()->void:
    if is_instance_valid(overlay):overlay.queue_free()
    overlay=null;widgets.clear()

func show_menu()->void:
    if is_instance_valid(overlay):return
    draft=values.duplicate();widgets.clear()
    overlay=Control.new();overlay.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);game.root_ui.add_child(overlay)
    var dim=ColorRect.new();dim.color=Color(0.005,0.02,0.035,0.87);dim.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);overlay.add_child(dim)
    var center=CenterContainer.new();center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);overlay.add_child(center)
    var panel=game._panel(center);panel.custom_minimum_size=Vector2(710,0)
    var column=game._vbox(panel,10)
    column.add_child(game._label("Grafik & Bildschirm",30))
    column.add_child(game._label("Deine Einstellungen gelten auch beim nächsten Start.",15,game.MUTED))
    var scroll=ScrollContainer.new();scroll.custom_minimum_size=Vector2(670,410);scroll.horizontal_scroll_mode=ScrollContainer.SCROLL_MODE_DISABLED;column.add_child(scroll)
    var rows=game._vbox(scroll,8);rows.size_flags_horizontal=Control.SIZE_EXPAND_FILL
    _option(rows,"Bildschirmmodus","display",["Fenster","Vollbild (randlos)"])
    _option(rows,"Renderauflösung","resolution",["Automatisch / native Auflösung","1280 × 720","1920 × 1080","2560 × 1440","3840 × 2160"])
    _option(rows,"Grafikqualität","quality",["Niedrig","Mittel","Hoch","Ultra"])
    _option(rows,"Bildratenlimit","fps",["Unbegrenzt","60 FPS","120 FPS","144 FPS","240 FPS"])
    _option(rows,"Kantenglättung","msaa",["Aus","2× MSAA","4× MSAA","8× MSAA"])
    _toggle(rows,"VSync","vsync")
    _toggle(rows,"Schatten","shadows")
    _toggle(rows,"Wasserreflexionen","reflections")
    _toggle(rows,"Umgebungsbeleuchtung","ambient")
    _toggle(rows,"Leuchteffekte","bloom")
    _toggle(rows,"Tiefenschärfe","dof")
    var hint=game._label("Die Auflösung verändert im Vollbild die 3D-Grafik; Menüs bleiben scharf.\nIm Fenstermodus bestimmt sie auch die Fenstergröße.",14,game.MUTED)
    hint.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;hint.custom_minimum_size.x=650;column.add_child(hint)
    var buttons=game._hbox(column,10)
    buttons.add_child(game._button("Standard",func():draft=DEFAULTS.duplicate();_refresh()))
    var spacer=Control.new();spacer.size_flags_horizontal=Control.SIZE_EXPAND_FILL;buttons.add_child(spacer)
    buttons.add_child(game._button("Abbrechen",close))
    var apply=game._button("Übernehmen",func():values=draft.duplicate();apply_display();apply_world();store();close(),true)
    buttons.add_child(apply);apply.grab_focus()

func _row(parent:Node,title:String)->HBoxContainer:
    var row=game._hbox(parent,16)
    var label=game._label(title,17);label.size_flags_horizontal=Control.SIZE_EXPAND_FILL;row.add_child(label)
    return row

func _option(parent:Node,title:String,key:String,options:Array)->void:
    var row=_row(parent,title);var button=OptionButton.new();button.custom_minimum_size=Vector2(330,38)
    for option in options:button.add_item(option)
    button.select(draft[key]);row.add_child(button);widgets[key]=button
    button.item_selected.connect(func(index:int):
        draft[key]=index
        if key=="quality":_preset(index);_refresh()
    )

func _toggle(parent:Node,title:String,key:String)->void:
    var row=_row(parent,title);var button=CheckButton.new();button.text="Aktiv";button.custom_minimum_size=Vector2(330,34)
    button.button_pressed=draft[key];row.add_child(button);widgets[key]=button
    button.toggled.connect(func(enabled:bool):draft[key]=enabled)

func _preset(index:int)->void:
    draft.quality=index;draft.msaa=[0,1,2,3][index]
    draft.shadows=index>0;draft.reflections=index>0;draft.ambient=index>=2;draft.bloom=index>0;draft.dof=index>=2

func _refresh()->void:
    for key in widgets:
        if widgets[key] is OptionButton:widgets[key].select(draft[key])
        else:widgets[key].set_pressed_no_signal(draft[key])
