extends Node3D

const PORT=27844
const MAX_PLAYERS=8
const FILAMENTS=[
    {"name":"Bambu Lab PLA Matte Schwarz","color":Color("292d35")},
    {"name":"Bambu Lab PLA Matte Weiß","color":Color("e9ebe5")},
    {"name":"Bambu Lab PLA Matte Mandarinorange","color":Color("f28a38")},
    {"name":"Bambu Lab PLA Matte Marineblau","color":Color("224e85")},
    {"name":"Bambu Lab PLA Matte Eisblau","color":Color("96d5e7")},
    {"name":"Bambu Lab PLA Matte Grasgrün","color":Color("61a75a")},
    {"name":"Bambu Lab PLA Matte Sakura Pink","color":Color("eaa9c6")},
    {"name":"Bambu Lab PLA Matte Zitronengelb","color":Color("eed052")},
    {"name":"Prusament PLA Galaxy Black","color":Color("454951")},
    {"name":"eSUN PLA+ Feuerrot","color":Color("e34842")}
]
const ITEM_NAMES=["Kein Item", "Lenkspule", "Turbo-Filament", "Schutzschild"]
const BOT_NAMES=["Spaghetti", "Layer King", "Nozzle Ninja", "Captain PLA", "Infill Phil", "Support Sam", "Benchy Bot"]
const INK=Color("091823")
const TEXT=Color("edf8fb")
const MUTED=Color("9ab5c3")
const ACCENT=Color("ffac65")
const AQUA=Color("67e6d1")
var world:RaceWorld
var camera:Camera3D
var boats:Dictionary={}
var players:Dictionary={}
var local_id:int=1
var selected_map:int=0
var selected_color:int=2
var player_name:String="Skipper"
var mode:String="menu"
var networked:bool=false
var elapsed:float=0
var countdown:float=3.5
var frame_clock:float=0
var net_timer:float=0
var menu_boat:RacingBoat
var ui:CanvasLayer
var root_ui:Control
var screen:Control
var status_label:Label
var color_label:Label
var map_label:Label
var hud_speed:Label
var hud_lap:Label
var hud_time:Label
var hud_rank:Label
var hud_item:Label
var hud_coin:Label
var hud_warning:Label
var countdown_label:Label
var boost_bar:ProgressBar
var leaderboard:Label
var minimap:Control
var name_edit:LineEdit
var address_edit:LineEdit
var port_edit:SpinBox
var upnp_check:CheckBox
var pause_layer:Control
var finish_shown:bool=false
var paused:bool=false
var pickup_nodes:Array[Node3D]=[]
var pickup_data:Array[Dictionary]=[]
var shots:Dictionary={}
var shot_id:int=0
var message_until:float=0
var message_text:String=""
var race_entries:Array=[]
var upnp_thread:Thread
var upnp_mapped:bool=false
var upnp_port:int=PORT
var config=ConfigFile.new()
var graphics:GraphicsSettings
var audio_player:AudioStreamPlayer
var music_player:AudioStreamPlayer
var music_volume:float=0.72
var music_track:int=-1
const MUSIC_TRACKS=[preload("res://assets/music/wake_runner.ogg"),preload("res://assets/music/northern_drift.ogg"),preload("res://assets/music/afterglow_circuit.ogg")]
var audio_muted:bool=false
var sound_volume:float=0.12
var test_args:PackedStringArray
var test_enabled:bool=false
var test_autodrive:bool=false
var test_end:float=0
var screenshot_path:String=""
var screenshot_taken:bool=false
var screenshot_time:float=6.0
var sequence_dir:String=""
var sequence_frame:int=0
var sequence_time:float=0.0
var quitting:bool=false
var connection_timer:float=0
var map_buttons:Array[Button]=[]
var color_buttons:Array[Button]=[]

func _ready()->void:
    config.load("user://settings.cfg")
    player_name=str(config.get_value("player","name","Skipper"))
    selected_color=clampi(int(config.get_value("player","color",2)),0,FILAMENTS.size()-1)
    selected_map=clampi(int(config.get_value("player","map",0)),0,2)
    audio_muted=bool(config.get_value("audio","muted",false))
    music_volume=clampf(float(config.get_value("audio","music_volume",0.72)),0.0,1.0)
    graphics=GraphicsSettings.new();graphics.setup(self);graphics.apply_display()
    _inputs()
    multiplayer.peer_disconnected.connect(_peer_left)
    multiplayer.connected_to_server.connect(_connected)
    multiplayer.connection_failed.connect(_connection_failed)
    multiplayer.server_disconnected.connect(_server_left)
    camera=Camera3D.new();camera.fov=64;camera.far=1800;add_child(camera);camera.current=true
    var lens=CameraAttributesPractical.new();lens.dof_blur_far_enabled=true;lens.dof_blur_far_distance=550;lens.dof_blur_far_transition=250;lens.dof_blur_amount=0.06
    camera.attributes=lens
    get_viewport().size_changed.connect(func():graphics._render_resolution())
    ui=CanvasLayer.new();add_child(ui)
    root_ui=Control.new();root_ui.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);ui.add_child(root_ui)
    var theme=Theme.new();theme.default_font_size=18;root_ui.theme=theme
    _audio_setup()
    _menu_world()
    _show_menu()
    _parse_test_args()

func _inputs()->void:
    var bindings={"accelerate":[KEY_W,KEY_UP],"brake":[KEY_S,KEY_DOWN],"left":[KEY_A,KEY_LEFT],"right":[KEY_D,KEY_RIGHT],"boost":[KEY_SHIFT],"item":[KEY_SPACE,KEY_E],"reset":[KEY_R],"pause":[KEY_ESCAPE],"mute":[KEY_M]}
    for action in bindings:
        InputMap.add_action(action)
        for key in bindings[action]:
            var event=InputEventKey.new();event.physical_keycode=key;InputMap.action_add_event(action,event)
    for pair in [["accelerate",JOY_BUTTON_A],["boost",JOY_BUTTON_LEFT_SHOULDER],["item",JOY_BUTTON_X],["reset",JOY_BUTTON_Y],["pause",JOY_BUTTON_START]]:
        var event=InputEventJoypadButton.new();event.button_index=pair[1];InputMap.action_add_event(pair[0],event)

func _style(bg:Color,border:Color=Color.TRANSPARENT,radius:int=12)->StyleBoxFlat:
    var s=StyleBoxFlat.new();s.bg_color=bg;s.border_color=border
    s.set_border_width_all(1 if border.a>0 else 0);s.set_corner_radius_all(radius)
    s.content_margin_left=18;s.content_margin_right=18;s.content_margin_top=13;s.content_margin_bottom=13
    return s

func _label(text:String,size:int=18,color:Color=TEXT)->Label:
    var l=Label.new();l.text=text;l.add_theme_font_size_override("font_size",size);l.add_theme_color_override("font_color",color)
    return l

func _button(text:String,fn:Callable,primary:bool=false)->Button:
    var b=Button.new();b.text=text;b.custom_minimum_size=Vector2(0,48);b.mouse_default_cursor_shape=Control.CURSOR_POINTING_HAND
    b.add_theme_color_override("font_color",INK if primary else TEXT)
    b.add_theme_color_override("font_hover_color",INK if primary else TEXT)
    b.add_theme_color_override("font_pressed_color",INK if primary else TEXT)
    b.add_theme_color_override("font_focus_color",INK if primary else TEXT)
    b.add_theme_stylebox_override("normal",_style(ACCENT if primary else Color("18323e"),Color("38505a")))
    b.add_theme_stylebox_override("hover",_style(Color("ffbf85") if primary else Color("284c59"),AQUA))
    b.add_theme_stylebox_override("pressed",_style(Color("e58d48") if primary else Color("0c212d"),ACCENT))
    b.add_theme_stylebox_override("focus",_style(Color(0,0,0,0),AQUA))
    b.pressed.connect(fn);return b

func _vbox(parent:Node,separation:int=12)->VBoxContainer:
    var v=VBoxContainer.new();v.add_theme_constant_override("separation",separation);parent.add_child(v);return v

func _hbox(parent:Node,separation:int=12)->HBoxContainer:
    var h=HBoxContainer.new();h.add_theme_constant_override("separation",separation);parent.add_child(h);return h

func _panel(parent:Node,bg:Color=Color(0.025,0.08,0.12,0.92))->PanelContainer:
    var p=PanelContainer.new();p.add_theme_stylebox_override("panel",_style(bg,Color(0.4,0.7,0.75,0.23),18));parent.add_child(p);return p

func _clear_screen()->void:
    if is_instance_valid(screen):screen.queue_free()
    screen=Control.new();screen.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);root_ui.add_child(screen)
    screen.mouse_filter=Control.MOUSE_FILTER_IGNORE

func _save_settings()->void:
    config.set_value("player","name",player_name);config.set_value("player","color",selected_color)
    config.set_value("player","map",selected_map);config.set_value("audio","muted",audio_muted)
    config.set_value("audio","music_volume",music_volume)
    config.save("user://settings.cfg")

func _menu_world()->void:
    if is_instance_valid(world):world.queue_free()
    world=RaceWorld.new();add_child(world);world.build(selected_map)
    graphics.apply_world()
    _set_music(selected_map)
    if is_instance_valid(menu_boat):menu_boat.queue_free()
    menu_boat=RacingBoat.new();add_child(menu_boat)
    menu_boat.setup(0,"",selected_color,FILAMENTS[selected_color].color)
    menu_boat.position=world.samples[0]+Vector3(0,0,10)
    menu_boat.scale=Vector3(2,2,2)
    menu_boat.name_tag.visible=false
    camera.position=menu_boat.position+Vector3(-11,7.5,12)
    camera.look_at(menu_boat.position+Vector3(-3,1.5,0))

func _show_menu()->void:
    mode="menu";_clear_screen();map_buttons.clear();color_buttons.clear()
    var shade=ColorRect.new();shade.color=Color(0.01,0.04,0.08,0.32);shade.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
    shade.mouse_filter=Control.MOUSE_FILTER_IGNORE;screen.add_child(shade)
    var margin=MarginContainer.new();margin.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
    margin.add_theme_constant_override("margin_left",38);margin.add_theme_constant_override("margin_top",25)
    margin.add_theme_constant_override("margin_right",38);margin.add_theme_constant_override("margin_bottom",25);screen.add_child(margin)
    var layout=_vbox(margin,12)
    var header=_hbox(layout)
    header.add_child(_label("◈  BENCHY RACERS",25))
    var spacer=Control.new();spacer.size_flags_horizontal=Control.SIZE_EXPAND_FILL;header.add_child(spacer)
    header.add_child(_label("WASSER. FILAMENT. VOLLGAS.",14,AQUA))
    header.add_child(_button("Grafik",graphics.show_menu))
    header.add_child(_button("Beenden",_quit_game))
    var content=_hbox(layout,28);content.size_flags_vertical=Control.SIZE_EXPAND_FILL
    var left=_panel(content);left.custom_minimum_size.x=550
    var scroll=ScrollContainer.new();scroll.horizontal_scroll_mode=ScrollContainer.SCROLL_MODE_DISABLED;left.add_child(scroll)
    var options=_vbox(scroll,14);options.size_flags_horizontal=Control.SIZE_EXPAND_FILL
    var eyebrow=_label("DEIN NÄCHSTES RENNEN",14,AQUA);options.add_child(eyebrow)
    var title=_label("Kleine Boote.\nGroße Rennen.",39);options.add_child(title)
    var nr=_hbox(options);nr.add_child(_label("Skipper",16,MUTED))
    name_edit=LineEdit.new();name_edit.text=player_name;name_edit.max_length=20;name_edit.placeholder_text="Dein Name"
    name_edit.size_flags_horizontal=Control.SIZE_EXPAND_FILL;name_edit.custom_minimum_size.y=42;nr.add_child(name_edit)
    options.add_child(_label("01  STRECKE",14,MUTED))
    var maps=_hbox(options,8)
    for i in range(3):
        var idx=i
        var b=_button(RaceWorld.MAP_NAMES[i],func():_select_map(idx));b.size_flags_horizontal=Control.SIZE_EXPAND_FILL
        b.add_theme_font_size_override("font_size",16);maps.add_child(b);map_buttons.append(b)
    map_label=_label(RaceWorld.MAP_DESCRIPTIONS[selected_map],14,MUTED);options.add_child(map_label)
    options.add_child(_label("02  FILAMENT",14,MUTED))
    var colors=GridContainer.new();colors.columns=10;colors.add_theme_constant_override("h_separation",8);options.add_child(colors)
    for i in range(FILAMENTS.size()):
        var idx=i
        var b=Button.new();b.custom_minimum_size=Vector2(40,40);b.tooltip_text=FILAMENTS[i].name;b.mouse_default_cursor_shape=Control.CURSOR_POINTING_HAND
        b.add_theme_stylebox_override("normal",_style(FILAMENTS[i].color,Color("536b79"),20))
        b.add_theme_stylebox_override("hover",_style(FILAMENTS[i].color,Color.WHITE,20))
        b.add_theme_stylebox_override("focus",_style(Color.TRANSPARENT,AQUA,20))
        b.pressed.connect(func():_select_color(idx));colors.add_child(b);color_buttons.append(b)
    color_label=_label(FILAMENTS[selected_color].name,16);options.add_child(color_label)
    options.add_child(HSeparator.new())
    var actions=_hbox(options,8)
    var solo=_button("Solo-Rennen",_solo,true);solo.size_flags_horizontal=Control.SIZE_EXPAND_FILL;actions.add_child(solo)
    var host=_button("Online hosten",_host);host.size_flags_horizontal=Control.SIZE_EXPAND_FILL;actions.add_child(host)
    var connection=_hbox(options,8)
    address_edit=LineEdit.new();address_edit.placeholder_text="IP oder Hostname des Gastgebers";address_edit.text=str(config.get_value("network","address",""))
    address_edit.size_flags_horizontal=Control.SIZE_EXPAND_FILL;address_edit.custom_minimum_size.y=44;connection.add_child(address_edit)
    connection.add_child(_button("Beitreten",_join))
    var advanced=_hbox(options,12)
    advanced.add_child(_label("UDP-Port",14,MUTED))
    port_edit=SpinBox.new();port_edit.min_value=1024;port_edit.max_value=65535;port_edit.value=PORT;port_edit.custom_minimum_size.x=115;advanced.add_child(port_edit)
    upnp_check=CheckBox.new();upnp_check.text="Routerfreigabe per UPnP";upnp_check.button_pressed=true;upnp_check.add_theme_font_size_override("font_size",14);advanced.add_child(upnp_check)
    status_label=_label("Online: bis zu 8 Skipper · Gastgeber startet das Rennen",14,MUTED)
    status_label.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;options.add_child(status_label)
    var right=Control.new();right.size_flags_horizontal=Control.SIZE_EXPAND_FILL;content.add_child(right)
    var boat_caption=_panel(right,Color(0.025,0.08,0.12,0.78));boat_caption.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_WIDE)
    boat_caption.offset_top=-144
    var info=_vbox(boat_caption,8);info.add_child(_label("DAS ORIGINAL. DEIN FILAMENT.",14,AQUA))
    info.add_child(_label("3DBenchy",32));info.add_child(_label("Drift · Boost · Lenkspulen · Makercoins",16,MUTED))
    var footer=_hbox(layout)
    footer.add_child(_label("WASD / Pfeile  Steuern     Shift  Boost     Leertaste  Item     R  Zurücksetzen",14,TEXT))
    var fs=Control.new();fs.size_flags_horizontal=Control.SIZE_EXPAND_FILL;footer.add_child(fs)
    footer.add_child(_button("Spielregeln",_help))
    _update_selection()

func _update_selection()->void:
    for i in range(map_buttons.size()):map_buttons[i].add_theme_stylebox_override("normal",_style(Color("286052") if i==selected_map else Color("18323e"),AQUA if i==selected_map else Color("38505a")))
    for i in range(color_buttons.size()):
        color_buttons[i].text="●" if i==selected_color else ""
        color_buttons[i].add_theme_color_override("font_color",Color.WHITE if FILAMENTS[i].color.get_luminance()<0.5 else INK)

func _select_map(idx:int)->void:
    selected_map=idx;_menu_world();map_label.text=RaceWorld.MAP_DESCRIPTIONS[idx];_update_selection();_save_settings()

func _select_color(idx:int)->void:
    selected_color=idx;color_label.text=FILAMENTS[idx].name
    menu_boat.material.set_shader_parameter("filament_color",FILAMENTS[idx].color)
    menu_boat.material.set_shader_parameter("galaxy",1.0 if idx==8 else 0.0)
    _update_selection();_save_settings()

func _read_player()->void:
    if is_instance_valid(name_edit):player_name=name_edit.text.strip_edges().left(20)
    if player_name.is_empty():player_name="Skipper"
    _save_settings()

func _solo()->void:
    _read_player();networked=false;local_id=1
    players={1:{"name":player_name,"color":selected_color}}
    _start()

func _host()->void:
    _read_player()
    var p=int(port_edit.value) if is_instance_valid(port_edit) else PORT
    var peer=ENetMultiplayerPeer.new();var err=peer.create_server(p,MAX_PLAYERS-1)
    if err!=OK:_status("Port %d konnte nicht geöffnet werden. Wähle einen anderen Port."%p);return
    multiplayer.multiplayer_peer=peer;networked=true;local_id=1;upnp_port=p
    players={1:{"name":player_name,"color":selected_color}}
    _show_lobby()
    if is_instance_valid(upnp_check) and upnp_check.button_pressed and not test_enabled:
        upnp_thread=Thread.new();upnp_thread.start(_map_router.bind(p))

func _map_router(p:int)->void:
    var upnp=UPNP.new();var result=upnp.discover(2200,2,"InternetGatewayDevice")
    var ip=""
    var success=false
    if result==UPNP.UPNP_RESULT_SUCCESS and upnp.get_gateway() and upnp.get_gateway().is_valid_gateway():
        ip=upnp.query_external_address()
        success=upnp.add_port_mapping(p,p,"Benchy Racers","UDP",3600)==UPNP.UPNP_RESULT_SUCCESS
    call_deferred("_router_ready",success,ip)

func _router_ready(success:bool,ip:String)->void:
    upnp_mapped=success
    if upnp_thread and upnp_thread.is_started():upnp_thread.wait_to_finish()
    if mode!="lobby":return
    if success:_status("Internetadresse: %s:%d · Routerfreigabe aktiv. Diese Adresse an deine Freunde geben."%[ip,upnp_port])
    else:_status("UPnP nicht verfügbar. Im Router UDP %d an diesen PC freigeben und Freunden deine öffentliche IP geben. Bei CGNAT: gemeinsames VPN verwenden."%upnp_port)

func _join()->void:
    _read_player()
    var host=address_edit.text.strip_edges() if is_instance_valid(address_edit) else "127.0.0.1"
    var p=int(port_edit.value) if is_instance_valid(port_edit) else PORT
    if host.is_empty():_status("Gib die IP oder den Hostnamen des Gastgebers ein.");return
    # Accept host:port as copied from the host's lobby.
    if host.count(":")==1:
        var split=host.split(":");host=split[0]
        if split[1].is_valid_int():p=clampi(int(split[1]),1024,65535)
    var peer=ENetMultiplayerPeer.new();var err=peer.create_client(host,p)
    if err!=OK:_status("Verbindung konnte nicht erstellt werden. Adresse prüfen.");return
    config.set_value("network","address",host);config.save("user://settings.cfg")
    multiplayer.multiplayer_peer=peer;networked=true;mode="connecting";connection_timer=0
    _status("Verbinde mit %s:%d … (Esc zum Abbrechen)"%[host,p])

func _connected()->void:
    local_id=multiplayer.get_unique_id()
    register_player.rpc_id(1,player_name,selected_color)

@rpc("any_peer","call_remote","reliable")
func register_player(nick:String,col:int)->void:
    if not multiplayer.is_server():return
    var id=multiplayer.get_remote_sender_id()
    if mode!="lobby" or players.size()>=MAX_PLAYERS:
        reject_join.rpc_id(id,"Ein Rennen läuft bereits. Nach dem Rennen erneut beitreten.");return
    players[id]={"name":nick.strip_edges().left(20) if not nick.strip_edges().is_empty() else "Skipper","color":clampi(col,0,FILAMENTS.size()-1)}
    _broadcast_lobby()

@rpc("authority","call_remote","reliable")
func reject_join(reason:String)->void:
    _disconnect();_show_menu();_status(reason)

func _broadcast_lobby()->void:
    lobby_state.rpc(players,selected_map)
    _show_lobby()

@rpc("authority","call_remote","reliable")
func lobby_state(entries:Dictionary,track:int)->void:
    players=entries;selected_map=clampi(track,0,2);mode="lobby";_menu_world();_show_lobby()

func _show_lobby()->void:
    mode="lobby";_clear_screen()
    var center=CenterContainer.new();center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);screen.add_child(center)
    var panel=_panel(center);panel.custom_minimum_size=Vector2(680,0)
    var v=_vbox(panel,16)
    v.add_child(_label("ONLINE-LOBBY",14,AQUA));v.add_child(_label("Alle an Bord?",38))
    v.add_child(_label("%s · 3 Runden · %d / %d Skipper"%[RaceWorld.MAP_NAMES[selected_map],players.size(),MAX_PLAYERS],18,MUTED))
    for id in players:
        var row=_hbox(v);var p=players[id]
        var swatch=ColorRect.new();swatch.color=FILAMENTS[int(p.color)].color;swatch.custom_minimum_size=Vector2(26,26);row.add_child(swatch)
        row.add_child(_label(str(p.name)+( "  · Gastgeber" if int(id)==1 else ""),22))
    v.add_child(HSeparator.new())
    status_label=_label("Gastgeber-IP teilen · UDP-Port %d\nFehlende Fahrerplätze bis 4 werden mit Bots gefüllt."%upnp_port,16,MUTED)
    status_label.autowrap_mode=TextServer.AUTOWRAP_WORD_SMART;v.add_child(status_label)
    if multiplayer.is_server():
        var addresses=[]
        for a in IP.get_local_addresses():
            if ":" not in a and not a.begins_with("127.") and not a.begins_with("169."):addresses.append(a)
        v.add_child(_label("Lokale Adresse: "+", ".join(addresses),14,MUTED))
        var tracks=_hbox(v)
        for i in range(3):
            var idx=i
            tracks.add_child(_button(RaceWorld.MAP_NAMES[i],func():selected_map=idx;_menu_world();_broadcast_lobby()))
        v.add_child(_button("Rennen starten",_start,true))
    else:v.add_child(_label("Der Gastgeber startet das Rennen.",18,AQUA))
    v.add_child(_button("Lobby verlassen",_return_menu))

func _status(text:String)->void:
    if is_instance_valid(status_label):status_label.text=text

func _start()->void:
    var entries:Array=[]
    for id in players:entries.append({"id":int(id),"name":players[id].name,"color":int(players[id].color),"bot":false})
    var bot_count=maxi(0,4-entries.size())
    for i in range(bot_count):entries.append({"id":-i-1,"name":BOT_NAMES[i],"color":(i*3+4)%FILAMENTS.size(),"bot":true})
    if networked:begin_race.rpc(selected_map,entries)
    else:begin_race(selected_map,entries)

@rpc("authority","call_local","reliable")
func begin_race(track:int,entries:Array)->void:
    selected_map=clampi(track,0,2);race_entries=entries
    _set_music(selected_map,true)
    _clear_race()
    if is_instance_valid(world):world.queue_free()
    if is_instance_valid(menu_boat):menu_boat.queue_free()
    world=RaceWorld.new();add_child(world);world.build(selected_map)
    graphics.apply_world()
    for i in range(entries.size()):
        var entry=entries[i];var b=RacingBoat.new();b.name="Boat_"+str(entry.id);add_child(b)
        b.setup(int(entry.id),str(entry.name),int(entry.color),FILAMENTS[int(entry.color)].color,bool(entry.bot))
        b.name_tag.visible=int(entry.id)!=local_id
        var forward=world.tangent(0);var side=Vector3(-forward.z,0,forward.x)
        var pos=world.samples[0]-forward*(5+int(i/2)*5)+side*(-2.4 if i%2==0 else 2.4)
        b.put_on_grid(pos,atan2(-forward.x,-forward.z));boats[int(entry.id)]=b
    _spawn_pickups()
    world.external_clock=true;world.clock=0
    elapsed=0;countdown=3.5;net_timer=0;mode="race";finish_shown=false;paused=false
    _show_hud()
    var mine:RacingBoat=boats[local_id]
    camera.position=mine.position+Vector3(sin(mine.heading)*11,7,cos(mine.heading)*11)
    camera.look_at(mine.position+Vector3(0,1.5,0))

func _clear_race()->void:
    for b in boats.values():
        for w in b.trail:
            if is_instance_valid(w):w.queue_free()
        b.queue_free()
    boats.clear()
    for p in pickup_nodes:if is_instance_valid(p):p.queue_free()
    pickup_nodes.clear();pickup_data.clear()
    for shot in shots.values():if is_instance_valid(shot.node):shot.node.queue_free()
    shots.clear()

func _spawn_pickups()->void:
    for gate in range(1,16):
        var idx=int(float(gate)/16*world.samples.size())
        var p=world.samples[idx];var f=world.tangent(idx);var side=Vector3(-f.z,0,f.x)
        if gate%3==0:
            for s in [-1,0,1]:_pickup(p+side*s*6,1)
        else:
            for k in range(3):_pickup(p+f*k*4+side*(3 if gate%2==0 else -3),0)

func _spool_node(color:Color,size:float=1.0)->Node3D:
    var n=Node3D.new()
    world.cylinder(Vector3.ZERO,0.58*size,0.7*size,color,n)
    for s in [-1,1]:world.cylinder(Vector3(0,0.42*size*s,0),0.85*size,0.16*size,Color("ecedef"),n)
    world.cylinder(Vector3(0,0.53*size,0),0.22*size,0.03*size,Color("283847"),n)
    return n

func _pickup(pos:Vector3,kind:int)->void:
    var n=Node3D.new();n.position=pos+Vector3(0,1.55,0);add_child(n)
    if kind==0:
        var coin=world.cylinder(Vector3.ZERO,0.70,0.20,Color("ffce5a"),n);coin.rotation.x=PI/2
        var label=Label3D.new();label.text="M";label.font_size=100;label.pixel_size=0.011;label.outline_size=0
        label.position.z=0.13;label.modulate=Color("9c671c");n.add_child(label)
        var back=Label3D.new();back.text="M";back.font_size=100;back.pixel_size=0.011;back.outline_size=0
        back.rotation.y=PI;back.position.z=-0.13;back.modulate=Color("9c671c");n.add_child(back)
    else:
        var spool=_spool_node(Color("f19e55"),1.35);spool.rotation.z=0.3;n.add_child(spool)
        var label=Label3D.new();label.text="?";label.position.y=1.65;label.font_size=110;label.pixel_size=0.016
        label.modulate=AQUA;label.billboard=BaseMaterial3D.BILLBOARD_ENABLED;n.add_child(label)
    pickup_nodes.append(n);pickup_data.append({"pos":pos,"kind":kind,"available":true,"respawn":0.0})

func _show_hud()->void:
    _clear_screen()
    var top=MarginContainer.new();top.set_anchors_and_offsets_preset(Control.PRESET_TOP_WIDE);top.offset_left=26;top.offset_right=-26;top.offset_top=22;screen.add_child(top)
    var row=_hbox(top,16)
    var position_panel=_panel(row);var pv=_vbox(position_panel,0)
    hud_rank=_label("1 / 4",38);pv.add_child(hud_rank);pv.add_child(_label("POSITION",13,AQUA))
    var info=_panel(row);var iv=_vbox(info,5)
    hud_lap=_label("RUNDE 1 / 3",22);iv.add_child(hud_lap);iv.add_child(_label(RaceWorld.MAP_NAMES[selected_map],14,MUTED))
    var gap=Control.new();gap.size_flags_horizontal=Control.SIZE_EXPAND_FILL;row.add_child(gap)
    var time_panel=_panel(row);var tv=_vbox(time_panel,2)
    hud_time=_label("00:00.00",28);tv.add_child(hud_time);tv.add_child(_label("RENNZEIT",13,MUTED))
    var pause_button=_button("Pause",_toggle_pause);pause_button.focus_mode=Control.FOCUS_NONE;row.add_child(pause_button)
    var ranks_panel=_panel(screen,Color(0.025,0.08,0.12,0.75));ranks_panel.position=Vector2(26,142);ranks_panel.custom_minimum_size=Vector2(210,0)
    leaderboard=_label("",16,TEXT);ranks_panel.add_child(leaderboard)
    var mini_panel=_panel(screen,Color(0.025,0.08,0.12,0.8));mini_panel.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_LEFT)
    mini_panel.offset_left=26;mini_panel.offset_top=-244;mini_panel.offset_right=264;mini_panel.offset_bottom=-24
    minimap=preload("res://minimap.gd").new();minimap.custom_minimum_size=Vector2(202,184)
    minimap.race_world=world;minimap.boats=boats;minimap.local_id=local_id;minimap.colours=FILAMENTS;mini_panel.add_child(minimap)
    var speed_panel=_panel(screen,Color(0.025,0.08,0.12,0.88));speed_panel.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_RIGHT)
    speed_panel.offset_left=-280;speed_panel.offset_top=-205;speed_panel.offset_right=-26;speed_panel.offset_bottom=-24
    var sv=_vbox(speed_panel,5)
    hud_speed=_label("0",55);sv.add_child(hud_speed);sv.add_child(_label("KM/H    ·    SHIFT FÜR BOOST",13,MUTED))
    boost_bar=ProgressBar.new();boost_bar.custom_minimum_size=Vector2(200,8);boost_bar.show_percentage=false
    boost_bar.add_theme_stylebox_override("background",_style(Color("152f3c"),Color.TRANSPARENT,4))
    boost_bar.add_theme_stylebox_override("fill",_style(AQUA,Color.TRANSPARENT,4));sv.add_child(boost_bar)
    var center_bottom=CenterContainer.new();center_bottom.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_WIDE)
    center_bottom.offset_top=-130;center_bottom.offset_bottom=-24;screen.add_child(center_bottom)
    var item_panel=_panel(center_bottom);var item_row=_hbox(item_panel,24)
    var cv=_vbox(item_row,3);hud_coin=_label("◈  0 / 10",25,Color("ffda7b"));cv.add_child(hud_coin);cv.add_child(_label("MAKERCOINS",13,MUTED))
    var itv=_vbox(item_row,3);hud_item=_label("Kein Item",23);itv.add_child(hud_item);itv.add_child(_label("LEERTASTE / E  ·  EINSETZEN",13,MUTED))
    var countdown_container=CenterContainer.new();countdown_container.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
    countdown_container.mouse_filter=Control.MOUSE_FILTER_IGNORE;screen.add_child(countdown_container)
    countdown_label=_label("3",110);countdown_label.add_theme_color_override("font_shadow_color",Color(0,0,0,0.6))
    countdown_label.add_theme_constant_override("shadow_offset_x",4);countdown_label.add_theme_constant_override("shadow_offset_y",4)
    countdown_container.add_child(countdown_label)
    hud_warning=_label("",22,ACCENT);hud_warning.horizontal_alignment=HORIZONTAL_ALIGNMENT_CENTER
    hud_warning.set_anchors_and_offsets_preset(Control.PRESET_CENTER_TOP);hud_warning.offset_top=137;hud_warning.offset_left=-300;hud_warning.offset_right=300
    screen.add_child(hud_warning)

func _ranked()->Array:
    var list=boats.values()
    list.sort_custom(func(a:RacingBoat,b:RacingBoat):
        if a.finished and b.finished:return a.finish_time<b.finish_time
        if a.finished!=b.finished:return a.finished
        return _progress(a)>_progress(b))
    return list

func _progress(b:RacingBoat)->float:
    var gates_done=b.checkpoint-1 if b.checkpoint>0 else 15
    var gate_idx=int(float(gates_done)/16*world.samples.size())
    var fraction=clampf(float(posmod(b.route_index-gate_idx,world.samples.size()))/(float(world.samples.size())/16),0,0.999)
    return b.lap*16+gates_done+fraction

func _format_time(t:float)->String:
    return "%02d:%05.2f"%[int(t)/60,fmod(t,60.0)]

func _process(dt:float)->void:
    frame_clock+=dt
    _audio_fill()
    if mode in ["menu","lobby","connecting"]:
        if is_instance_valid(menu_boat):
            menu_boat.heading=frame_clock*0.15;menu_boat.rotation.y=menu_boat.heading
            menu_boat.simulate_buoyancy(minf(dt,0.04),world.clock,world.wave_scale)
            menu_boat.animate(dt,world.clock,world.wave_scale)
            camera.position=menu_boat.position+Vector3(-11,7.5+sin(frame_clock*0.3)*0.4,12)
            camera.look_at(menu_boat.position+Vector3(-3,1.5,0))
        if mode=="connecting":
            connection_timer+=dt
            if connection_timer>12:_connection_failed()
    if mode in ["race","results"]:
        for b in boats.values():
            b.animate(dt,world.clock,world.wave_scale)
            b.name_tag.visible=b.peer_id!=local_id and b.position.distance_to(camera.position)>9.0 and camera.global_transform.basis.z.dot(b.name_tag.global_position-camera.position)<-1.0
        for i in range(pickup_nodes.size()):
            var n=pickup_nodes[i];n.visible=pickup_data[i].available
            n.rotation.y=frame_clock*1.7+i;n.position.y=1.65+sin(frame_clock*2.3+i)*0.25
        _camera_follow(dt)
        if mode=="race":_update_hud()
    if test_enabled:_test_tick(dt)

func _camera_follow(dt:float)->void:
    if not boats.has(local_id):return
    var b:RacingBoat=boats[local_id]
    var forward=Vector3(-sin(b.heading),0,-cos(b.heading))
    var target=b.position-forward*(7.0+absf(b.speed)*0.018)+Vector3(0,3.8,0)
    if "--wake-preview" in test_args:target=b.position-forward*18.0+Vector3(0,9,0)
    camera.position=camera.position.lerp(target,1-exp(-dt*8))
    var look=b.position+forward*7.5+Vector3(0,0.9,0)
    camera.look_at(look)
    camera.fov=lerpf(camera.fov,72.0 if b.speed>35 else 64.0,dt*3)
    camera.rotation.z=lerpf(camera.rotation.z,-b.controls.y*0.025,dt*5)
    if b.stun>0:camera.position+=Vector3(sin(frame_clock*43),cos(frame_clock*47),0)*0.08

func _get_controls()->Vector3:
    if paused:return Vector3.ZERO
    if test_autodrive and boats.has(local_id):return boats[local_id].ai_input(world,elapsed)
    var gas=Input.get_action_strength("accelerate")-Input.get_action_strength("brake")
    var steering=Input.get_action_strength("right")-Input.get_action_strength("left")
    var pads=Input.get_connected_joypads()
    if pads.size()>0:
        var pad=pads[0]
        var x=Input.get_joy_axis(pad,JOY_AXIS_LEFT_X)
        if absf(x)>0.12:steering=x
        var rt=Input.get_joy_axis(pad,JOY_AXIS_TRIGGER_RIGHT)
        var lt=Input.get_joy_axis(pad,JOY_AXIS_TRIGGER_LEFT)
        if rt>0.1:gas=maxf(gas,rt)
        if lt>0.1:gas=-lt
    return Vector3(gas,steering,Input.get_action_strength("boost"))

func _physics_process(dt:float)->void:
    if mode!="race":return
    if paused and not networked:return
    if countdown>0:
        countdown-=dt
        world.clock=3.5-countdown
        if not networked or multiplayer.is_server():
            for b in boats.values():b.simulate_buoyancy(dt,world.clock,world.wave_scale)
        if networked and multiplayer.is_server() and countdown<=0:sync_clock.rpc(0.0)
        return
    elapsed+=dt
    world.clock=elapsed+3.5
    var controls=_get_controls()
    if boats.has(local_id):boats[local_id].controls=controls
    if not networked or multiplayer.is_server():
        for b in boats.values():
            if b.is_bot:
                b.controls=b.ai_input(world,elapsed)
                if b.held_item>0 and fmod(elapsed+absf(b.peer_id)*1.37,4)<dt:_use_item(b.peer_id)
            b.simulate(dt,world,elapsed)
        _collisions()
        _update_pickups(dt)
        _update_shots(dt)
    net_timer+=dt
    if networked and net_timer>=0.05:
        net_timer=0
        if multiplayer.is_server():send_snapshot.rpc(_snapshot(),elapsed,_pickup_flags(),_shot_snapshot())
        else:submit_input.rpc_id(1,controls)
    if boats.has(local_id) and boats[local_id].finished and not finish_shown:
        finish_shown=true;_show_results()

@rpc("authority","call_remote","reliable")
func sync_clock(t:float)->void:
    countdown=0;elapsed=t

@rpc("any_peer","call_remote","unreliable_ordered",1)
func submit_input(input:Vector3)->void:
    if not multiplayer.is_server() or mode!="race":return
    var id=multiplayer.get_remote_sender_id()
    if boats.has(id) and input.is_finite():boats[id].controls=Vector3(clampf(input.x,-1,1),clampf(input.y,-1,1),clampf(input.z,0,1))

func _snapshot()->Array:
    var data:Array=[]
    for b in boats.values():data.append([b.peer_id,b.position,b.heading,b.speed,b.boost,b.lap,b.checkpoint,b.finished,b.finish_time,b.coins,b.held_item,b.stun,b.shield,b.item_boost,b.route_index,b.off_track,b.controls,b.best_lap,b.hull_pitch,b.hull_roll,b.vertical_velocity])
    return data

func _pickup_flags()->Array:
    var flags:Array=[]
    for p in pickup_data:flags.append(p.available)
    return flags

func _shot_snapshot()->Array:
    var data:Array=[]
    for id in shots:data.append([id,shots[id].owner,shots[id].pos,shots[id].direction,shots[id].target,shots[id].life])
    return data

@rpc("authority","call_remote","unreliable_ordered",2)
func send_snapshot(data:Array,time:float,pickups:Array,projectiles:Array)->void:
    if mode not in ["race","results"]:return
    elapsed=time
    for row in data:
        if not boats.has(int(row[0])):continue
        var b:RacingBoat=boats[int(row[0])]
        b.target_position=row[1];b.target_heading=row[2];b.has_target=true
        b.speed=row[3];b.boost=row[4];b.lap=row[5];b.checkpoint=row[6];b.finished=row[7];b.finish_time=row[8]
        b.coins=row[9];b.held_item=row[10];b.stun=row[11];b.shield=row[12];b.item_boost=row[13];b.route_index=row[14]
        b.off_track=row[15];b.controls=row[16];b.best_lap=row[17]
        b.hull_pitch=row[18];b.hull_roll=row[19];b.vertical_velocity=row[20]
    for i in range(mini(pickups.size(),pickup_data.size())):pickup_data[i].available=bool(pickups[i])
    var alive:Array=[]
    for row in projectiles:
        var id=int(row[0]);alive.append(id)
        if not shots.has(id):_create_shot(id,int(row[1]),row[2],row[3],int(row[4]))
        shots[id].pos=row[2];shots[id].node.position=row[2]
    for id in shots.keys():
        if id not in alive:shots[id].node.queue_free();shots.erase(id)

func _collisions()->void:
    var all=boats.values()
    if world.nature_kit:
        for boat in all:
            if world.nature_kit.ground_height(Vector2(boat.position.x,boat.position.z))>0.3:
                var near=world.nearest(boat.position,boat.route_index)
                var toward=world.samples[near.index]-boat.position;toward.y=0
                if toward.length()>0.01:
                    var normal=toward.normalized()
                    boat.position+=normal*0.8
                    var impact=boat.velocity.dot(normal)
                    if impact<0:boat.velocity-=normal*impact*1.2;boat.speed*=0.72
    for i in range(all.size()):
        for j in range(i+1,all.size()):
            var a:RacingBoat=all[i];var b:RacingBoat=all[j]
            var delta=a.position-b.position;delta.y=0
            var distance=delta.length()
            if distance<2.2 and distance>0.01:
                var normal=delta/distance
                var push=normal*(2.2-distance)*0.5
                a.position+=push;b.position-=push
                var rel=(a.velocity-b.velocity).dot(normal)
                if rel<0:
                    a.velocity-=normal*rel*0.72;b.velocity+=normal*rel*0.72
                    a.speed*=0.97;b.speed*=0.97

func _update_pickups(dt:float)->void:
    for i in range(pickup_data.size()):
        var p=pickup_data[i]
        if not p.available:
            p.respawn-=dt
            if p.respawn<=0:p.available=true
            continue
        for b in boats.values():
            if b.finished or b.position.distance_to(p.pos)>2.6:continue
            if p.kind==0:
                if b.coins>=10:continue
                b.coins=mini(10,b.coins+1)
            else:
                if b.held_item>0:continue
                # Trailing racers get more offensive items, leaders more defensive ones.
                var rank=_ranked().find(b)
                var roll=randi()%100
                b.held_item=1 if roll<(58 if rank>1 else 38) else (2 if roll<80 else 3)
                if b.peer_id==local_id:_message(ITEM_NAMES[b.held_item]+" bereit!",1.8)
            p.available=false;p.respawn=9.0 if p.kind==0 else 7.0
            break

func _use_item(id:int)->void:
    if not boats.has(id) or countdown>0:return
    var b:RacingBoat=boats[id]
    if b.finished or b.stun>0:return
    var item=b.held_item
    if item==0:return
    b.held_item=0
    if item==1:
        var forward=Vector3(-sin(b.heading),0,-cos(b.heading))
        var target_id=0;var best=120.0
        for other in boats.values():
            if other.peer_id==id or other.finished:continue
            var delta=other.position-b.position
            if delta.length()<best and delta.normalized().dot(forward)>0.4:best=delta.length();target_id=other.peer_id
        shot_id+=1;_create_shot(shot_id,id,b.position+forward*3+Vector3(0,1,0),forward,target_id)
    elif item==2:b.item_boost=3.5
    elif item==3:b.shield=7.5

func _create_shot(id:int,owner:int,pos:Vector3,direction:Vector3,target:int)->void:
    var spool=_spool_node(Color("ff755f"),0.8);add_child(spool);spool.position=pos
    shots[id]={"node":spool,"owner":owner,"pos":pos,"direction":direction,"target":target,"life":5.0}

func _update_shots(dt:float)->void:
    for id in shots.keys():
        var s=shots[id];s.life-=dt
        if s.life<=0:s.node.queue_free();shots.erase(id);continue
        if s.target!=0 and boats.has(s.target) and not boats[s.target].finished:
            var delta=boats[s.target].position+Vector3(0,1,0)-s.pos
            s.direction=s.direction.lerp(delta.normalized(),dt*2.0).normalized()
        s.pos+=s.direction*58*dt;s.node.position=s.pos;s.node.rotation+=Vector3(0,dt*8,dt*6)
        for b in boats.values():
            if b.peer_id==s.owner or b.finished:continue
            if b.position.distance_to(s.pos-Vector3(0,1,0))<2.2:
                if b.shield>0:
                    b.shield=0
                    _race_event("shield",b.peer_id)
                else:
                    b.coins=0;b.stun=1.5;b.speed*=0.25;b.velocity*=0.25
                    _race_event("hit",b.peer_id)
                s.node.queue_free();shots.erase(id);break

func _race_event(kind:String,id:int)->void:
    if networked:race_event.rpc(kind,id)
    else:race_event(kind,id)

@rpc("authority","call_local","reliable")
func race_event(kind:String,id:int)->void:
    if not boats.has(id):return
    var b:RacingBoat=boats[id]
    if kind=="hit":
        _burst(b.position+Vector3(0,1,0),Color("ffbd64"))
        if id==local_id:_message("Getroffen! Alle Makercoins verloren.",2.0)
    elif kind=="shield":
        _burst(b.position+Vector3(0,1,0),AQUA)
        if id==local_id:_message("Schutzschild hat den Treffer abgefangen!",2.0)

func _burst(pos:Vector3,color:Color)->void:
    var p=CPUParticles3D.new();p.position=pos;p.amount=30;p.lifetime=0.75;p.one_shot=true
    p.explosiveness=1;p.direction=Vector3(0,1,0);p.spread=180;p.initial_velocity_min=3;p.initial_velocity_max=9
    p.gravity=Vector3(0,-10,0);p.scale_amount_min=0.1;p.scale_amount_max=0.4
    var mesh=SphereMesh.new();mesh.radius=0.3;mesh.height=0.6;mesh.radial_segments=4;mesh.rings=2;mesh.material=world.mat(color)
    p.mesh=mesh;add_child(p);p.emitting=true
    get_tree().create_timer(1.2).timeout.connect(func():if is_instance_valid(p):p.queue_free())

@rpc("any_peer","call_remote","reliable")
func request_item()->void:
    if multiplayer.is_server() and mode=="race":_use_item(multiplayer.get_remote_sender_id())

@rpc("any_peer","call_remote","reliable")
func request_reset()->void:
    var id=multiplayer.get_remote_sender_id()
    if multiplayer.is_server() and boats.has(id) and mode=="race":boats[id].reset_to_course(world)

func _message(text:String,duration:float=2)->void:
    message_text=text;message_until=frame_clock+duration

func _update_hud()->void:
    if not boats.has(local_id) or not is_instance_valid(hud_speed):return
    var b:RacingBoat=boats[local_id]
    var ranked=_ranked();var position=ranked.find(b)+1
    hud_speed.text=str(int(absf(b.speed)*3.6));hud_lap.text="RUNDE %d / 3"%mini(3,b.lap+1)
    hud_time.text=_format_time(elapsed);hud_rank.text="%d / %d"%[position,boats.size()]
    boost_bar.value=b.boost;hud_coin.text="◈  %d / 10"%b.coins
    hud_item.text=ITEM_NAMES[b.held_item];hud_item.modulate=AQUA if b.held_item>0 else MUTED
    var lines=[]
    for i in range(ranked.size()):
        var other=ranked[i]
        lines.append("%d   %s%s"%[i+1,"› " if other.peer_id==local_id else "",other.driver_name]+("  ✓" if other.finished else ""))
    leaderboard.text="\n".join(lines)
    countdown_label.text=str(int(ceil(countdown))) if countdown>0 else ("LOS!" if elapsed<0.75 else "")
    hud_warning.text=message_text if frame_clock<message_until else ("Strecke verlassen — folge den Bojen!" if b.off_track else ("SCHUTZSCHILD AKTIV" if b.shield>0 else ""))
    world.highlight_gate(b.checkpoint)

func _unhandled_input(event:InputEvent)->void:
    if event.is_action_pressed("mute"):
        audio_muted=not audio_muted;_save_settings()
    if event.is_action_pressed("pause"):
        if is_instance_valid(graphics.overlay):graphics.close();get_viewport().set_input_as_handled();return
        if mode=="connecting":_disconnect();_show_menu()
        elif mode=="race":_toggle_pause()
    if mode!="race" or paused:return
    if event.is_action_pressed("item"):
        if networked and not multiplayer.is_server():request_item.rpc_id(1)
        else:_use_item(local_id)
    if event.is_action_pressed("reset") and countdown<=0:
        if networked and not multiplayer.is_server():request_reset.rpc_id(1)
        elif boats.has(local_id):boats[local_id].reset_to_course(world)

func _toggle_pause()->void:
    if mode!="race":return
    paused=not paused
    if not paused:
        if is_instance_valid(pause_layer):pause_layer.queue_free()
        return
    pause_layer=Control.new();pause_layer.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);screen.add_child(pause_layer)
    var dim=ColorRect.new();dim.color=Color(0.01,0.04,0.07,0.6);dim.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);pause_layer.add_child(dim)
    var c=CenterContainer.new();c.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);pause_layer.add_child(c)
    var panel=_panel(c);panel.custom_minimum_size.x=480
    var v=_vbox(panel,14);v.add_child(_label("Kurze Verschnaufpause",30))
    if networked:v.add_child(_label("Das Online-Rennen läuft weiter.",16,ACCENT))
    v.add_child(_button("Weiterfahren",_toggle_pause,true))
    v.add_child(_button("Ton: "+("Aus" if audio_muted else "An"),func():audio_muted=not audio_muted;_save_settings();_toggle_pause()))
    var volume_row=_hbox(v);volume_row.add_child(_label("Musik",16,MUTED))
    var volume=HSlider.new();volume.min_value=0;volume.max_value=1;volume.step=0.05;volume.value=music_volume;volume.size_flags_horizontal=Control.SIZE_EXPAND_FILL
    volume.custom_minimum_size=Vector2(220,32);volume_row.add_child(volume)
    volume.value_changed.connect(func(value:float):music_volume=value;_save_settings())
    v.add_child(_button("Grafik & Bildschirm",graphics.show_menu))
    v.add_child(_button("Rennen verlassen",_return_menu))

func _fullscreen()->void:
    graphics.values.display=1-graphics.values.display;graphics.apply_display();graphics.store()

func _show_results()->void:
    # Keep the simulation alive so the remaining drivers can finish.
    var overlay=Control.new();overlay.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);screen.add_child(overlay)
    var dim=ColorRect.new();dim.color=Color(0.01,0.04,0.07,0.62);dim.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);overlay.add_child(dim)
    var c=CenterContainer.new();c.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);overlay.add_child(c)
    var panel=_panel(c);panel.custom_minimum_size.x=610
    var v=_vbox(panel,14)
    var b:RacingBoat=boats[local_id]
    var rank=_ranked().find(b)+1
    v.add_child(_label("ZIEL ERREICHT",14,AQUA))
    v.add_child(_label("Sieg für dein Benchy!" if rank==1 else "Sauber ins Ziel!",38))
    v.add_child(_label("Platz %d  ·  %s"%[rank,_format_time(b.finish_time)],27,ACCENT))
    v.add_child(_label("Beste Runde: "+_format_time(b.best_lap),18,MUTED))
    var result_label=_label("",20);v.add_child(result_label)
    var update_results=func():
        var lines=[];var ranked=_ranked()
        for i in range(ranked.size()):lines.append("%d.  %-20s  %s"%[i+1,ranked[i].driver_name,_format_time(ranked[i].finish_time) if ranked[i].finished else "fährt noch …"])
        result_label.text="\n".join(lines)
    update_results.call()
    var timer=Timer.new();timer.wait_time=0.5;timer.timeout.connect(update_results);overlay.add_child(timer);timer.start()
    if not networked:v.add_child(_button("Noch ein Rennen",func():_start(),true))
    elif multiplayer.is_server():v.add_child(_button("Zurück zur Lobby",_back_to_lobby,true))
    else:v.add_child(_label("Der Gastgeber kann eine neue Runde eröffnen.",16,AQUA))
    v.add_child(_button("Hauptmenü",_return_menu))

func _back_to_lobby()->void:
    return_lobby.rpc()

@rpc("authority","call_local","reliable")
func return_lobby()->void:
    _clear_race();_menu_world();_show_lobby()

func _help()->void:
    var overlay=Control.new();overlay.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);screen.add_child(overlay)
    var dim=ColorRect.new();dim.color=Color(0.01,0.04,0.07,0.8);dim.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);overlay.add_child(dim)
    var c=CenterContainer.new();c.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT);overlay.add_child(c)
    var panel=_panel(c);panel.custom_minimum_size.x=680
    var v=_vbox(panel,14);v.add_child(_label("So fährt man Benchy.",34))
    v.add_child(_label("Drei Runden. Alle Tore der Reihe nach passieren.\nDie grüne Markierung zeigt dir das nächste Tor.",20))
    v.add_child(_label("◈ Sammle bis zu 10 Makercoins.\nWirst du getroffen, gehen alle Coins verloren.",18,Color("ffda7b")))
    v.add_child(_label("Filamentrollen geben dir ein zufälliges Item:\n• Lenkspule: verfolgt einen Fahrer vor dir.\n• Turbo-Filament: 3,5 Sekunden Extraschub.\n• Schutzschild: blockt einen Treffer, hält 7,5 Sekunden.",18,AQUA))
    v.add_child(_label("WASD / Pfeile: Gas, Bremse und Lenkung\nShift: Boost · Leertaste / E: Item · R: letztes Tor\nEsc: Pause · M: Ton · Gamepad wird unterstützt.",18))
    v.add_child(_label("Online: Gastgeber öffnet eine Lobby und teilt seine IP.\nUDP 27844 muss erreichbar sein. UPnP versucht die Freigabe.\nWindows-Firewall beim ersten Start für das Spiel erlauben.",16,MUTED))
    v.add_child(_label("Original 3DBenchy: Daniel Norée / Creative Tools (CC0).\nFilamentfarben sind visuelle Annäherungen. Kein offizielles\nProdukt von Bambu Lab, Prusa oder eSUN.",14,MUTED))
    v.add_child(_button("Alles klar",func():overlay.queue_free(),true))

func _peer_left(id:int)->void:
    players.erase(id)
    if boats.has(id):
        boats[id].is_bot=true;boats[id].driver_name+=" (Bot)";boats[id].name_tag.text=boats[id].driver_name
    if networked and multiplayer.is_server() and mode=="lobby":_broadcast_lobby()

func _connection_failed()->void:
    _disconnect();_show_menu();_status("Gastgeber nicht erreichbar. IP, UDP-Port und Routerfreigabe prüfen.")

func _server_left()->void:
    _clear_race();_disconnect();_menu_world();_show_menu();_status("Der Gastgeber hat die Verbindung beendet.")

func _disconnect()->void:
    if multiplayer.multiplayer_peer:multiplayer.multiplayer_peer.close()
    multiplayer.multiplayer_peer=OfflineMultiplayerPeer.new();networked=false;local_id=1;players.clear()
    if upnp_thread and upnp_thread.is_started():upnp_thread.wait_to_finish()
    if upnp_mapped:
        var upnp=UPNP.new()
        if upnp.discover(1000,2,"InternetGatewayDevice")==UPNP.UPNP_RESULT_SUCCESS:upnp.delete_port_mapping(upnp_port,"UDP")
        upnp_mapped=false

func _return_menu()->void:
    _clear_race();_disconnect();paused=false;_menu_world();_show_menu()

func _notification(what:int)->void:
    if what==NOTIFICATION_WM_CLOSE_REQUEST:
        _quit_game()

func _quit_game()->void:
    if quitting:return
    quitting=true;test_enabled=false;mode="quitting"
    if is_instance_valid(audio_player):
        audio_player.stop();audio_player.stream=null
    if is_instance_valid(music_player):
        music_player.stop();music_player.stream=null
    _disconnect()
    # Give the audio mixer a moment to release its playback resources cleanly.
    _clear_race()
    if is_instance_valid(menu_boat):menu_boat.queue_free()
    if is_instance_valid(world):world.queue_free();world=null
    await get_tree().create_timer(0.2).timeout
    get_tree().quit()

func _exit_tree()->void:
    if is_instance_valid(audio_player):audio_player.stop()
    if is_instance_valid(music_player):music_player.stop()

func _audio_setup()->void:
    audio_player=AudioStreamPlayer.new();add_child(audio_player)
    var motor=preload("res://assets/motor.wav")
    motor.loop_mode=AudioStreamWAV.LOOP_FORWARD;motor.loop_begin=0;motor.loop_end=22050
    audio_player.stream=motor;audio_player.volume_db=-40;audio_player.play()
    music_player=AudioStreamPlayer.new();add_child(music_player);music_player.volume_db=-14
    for track in MUSIC_TRACKS:track.loop=true

func _set_music(track:int,restart:bool=false)->void:
    if not is_instance_valid(music_player):return
    if music_track!=track or restart:
        music_track=track;music_player.stream=MUSIC_TRACKS[track];music_player.play()

func _audio_fill()->void:
    if not is_instance_valid(audio_player):return
    var b:RacingBoat=boats.get(local_id)
    var rpm=absf(b.speed) if is_instance_valid(b) and mode=="race" else 0.0
    audio_player.pitch_scale=1.0+rpm*0.072
    audio_player.volume_db=(-34.0+rpm*0.18) if not audio_muted and mode=="race" and countdown<=0 else -80.0
    if is_instance_valid(music_player):
        var gain=music_volume*(0.32 if mode in ["menu","lobby","connecting"] else 0.7 if paused else 0.88)
        if audio_muted or quitting:gain=0
        music_player.volume_db=lerpf(music_player.volume_db,linear_to_db(maxf(0.0001,gain)),0.035)

func _parse_test_args()->void:
    test_args=OS.get_cmdline_user_args()
    if test_args.is_empty():return
    test_enabled="--test" in test_args
    test_autodrive="--autodrive" in test_args
    for arg in test_args:
        if arg.begins_with("--screenshot="):screenshot_path=arg.trim_prefix("--screenshot=")
        if arg.begins_with("--capture-at="):screenshot_time=float(arg.trim_prefix("--capture-at="))
        if arg.begins_with("--sequence="):
            sequence_dir=arg.trim_prefix("--sequence=");DirAccess.make_dir_recursive_absolute(sequence_dir)
        if arg.begins_with("--map="):selected_map=clampi(int(arg.trim_prefix("--map=")),0,2)
        if arg.begins_with("--color="):selected_color=clampi(int(arg.trim_prefix("--color=")),0,FILAMENTS.size()-1)
        if arg.begins_with("--duration="):test_end=float(arg.trim_prefix("--duration="))
    if test_end<=0:test_end=18
    if "--host" in test_args:
        upnp_check.button_pressed=false;_host()
    elif "--client" in test_args:
        address_edit.text="127.0.0.1";_join()
    elif "--solo" in test_args:
        _solo()
        if "--mechanics" in test_args:_test_mechanics()
    elif "--menu" in test_args:_menu_world();_show_menu()
    if "--graphics-menu" in test_args:graphics.show_menu()
    if "--test-display" in test_args:
        assert(DisplayServer.window_get_mode()==DisplayServer.WINDOW_MODE_FULLSCREEN)
        print("GRAPHICS_PASS fullscreen_start")
    if "--graphics-check" in test_args:_test_graphics()
    if "--wake-check" in test_args:_test_wake()
    if "--wake-preview" in test_args:
        for id in boats:
            if id!=local_id:boats[id].visible=false

func _test_wake()->void:
    var boat:RacingBoat=boats[local_id];var original=boat.position
    boat.speed=30;boat.wake_points.clear()
    for i in range(20):
        boat.position=original+Vector3(float(i)*.6,0,float(i*i)*.035)
        boat.heading=float(i)*.03;boat._update_wake(.06,float(i)*.06)
    var stern=boat.global_position+Vector3(sin(boat.heading),0,cos(boat.heading))*1.6
    assert(boat.wake_points[0].pos.distance_to(stern)<.001,"Wake must start at the world-space stern")
    assert(boat.wake.global_transform.is_equal_approx(Transform3D.IDENTITY),"World-space wake mesh must never inherit boat transforms")
    var wake_vertices:PackedVector3Array=boat.wake_mesh.surface_get_arrays(0)[Mesh.ARRAY_VERTEX]
    var center=(wake_vertices[6]+wake_vertices[8])*.5
    assert(Vector2(center.x-stern.x,center.z-stern.z).length()<.02,"Center foam must be directly behind the hull")
    var previous:Vector3=boat.wake_points[8].pos
    boat.heading+=1.0;boat.position+=Vector3(1,0,1);boat._update_wake(.06,1.3)
    assert(boat.wake_points[9].pos==previous,"Wake history must stay in world space during turns")
    assert(boat.wake_mesh.get_surface_count()==1 and boat.wake_points.size()>10)
    boat.speed=0;boat._update_wake(3.0,4.3)
    assert(boat.wake_points.is_empty() and not boat.wake.visible,"Stopped wake must dissolve over time")
    boat.position=original
    print("WAKE_PASS stern_alignment_world_space_history_turning_and_decay")

func _test_graphics()->void:
    var original=graphics.values.duplicate()
    graphics.draft=original.duplicate();graphics._preset(0);graphics.values=graphics.draft.duplicate();graphics.apply_world()
    assert(not world.environment.environment.ssr_enabled and not world.environment.environment.ssao_enabled)
    assert(get_viewport().msaa_3d==Viewport.MSAA_DISABLED)
    assert(world.water.mesh.subdivide_width==320)
    graphics._preset(3);graphics.values=graphics.draft.duplicate();graphics.apply_world()
    assert(world.environment.environment.ssao_enabled)
    if world.map_id==0:assert(world.water.material_override.get_shader_parameter("reflections"))
    else:assert(world.environment.environment.ssr_enabled)
    assert(get_viewport().msaa_3d==Viewport.MSAA_8X)
    assert(world.water.mesh.subdivide_width==1000)
    print("GRAPHICS_PASS quality_presets_and_effects")
    graphics.save_path="user://graphics-verification.cfg";graphics.values.display=0;graphics.values.resolution=2;graphics.store()
    var saved=ConfigFile.new();assert(saved.load(graphics.save_path)==OK)
    assert(saved.get_value("graphics","display")==0 and saved.get_value("graphics","resolution")==2)
    assert(saved.get_value("graphics","quality")==3)
    var original_config=config;config=saved
    var reloaded=GraphicsSettings.new();reloaded.setup(self)
    assert(reloaded.values.display==0 and reloaded.values.resolution==2 and reloaded.values.quality==3)
    config=original_config
    print("GRAPHICS_PASS saved_display_resolution_quality")
    DirAccess.remove_absolute(ProjectSettings.globalize_path(graphics.save_path))
    graphics.values=original;graphics.save_path="user://settings.cfg"
    for key in original:config.set_value("graphics",key,original[key])
    graphics.apply_world()

func _test_mechanics()->void:
    var a:RacingBoat=boats[1];var b:RacingBoat=boats[-1]
    var a_pos=a.position;var b_pos=b.position
    countdown=0
    a.coins=10;a.speed=0;a.controls=Vector3(1,0,0)
    # Straight, unobstructed section: the coin bonus affects the speed cap.
    a.position=world.samples[0]
    for i in range(240):
        a.position=world.samples[0]
        a.simulate(1.0/60,world,float(i)/60)
    assert(a.speed>=32.8,"Ten coins must increase maximum speed by six percent")
    print("MECHANIC_PASS coin_speed_bonus")
    a.held_item=2;_use_item(1)
    assert(a.item_boost>3 and a.held_item==0,"Turbo must be consumed and active")
    print("MECHANIC_PASS turbo")
    a.position=Vector3(0,0,0);a.heading=0;b.position=Vector3(0,0,-10);b.coins=10;b.held_item=3
    _use_item(-1);assert(b.shield>7)
    a.held_item=1;_use_item(1)
    for i in range(20):_update_shots(0.01)
    assert(b.shield==0 and b.coins==10,"Shield must absorb one projectile without losing coins")
    print("MECHANIC_PASS shield")
    a.held_item=1;_use_item(1)
    for i in range(20):_update_shots(0.01)
    assert(b.coins==0 and b.stun>0,"Unshielded hit must remove all coins and stun")
    print("MECHANIC_PASS projectile_hit_coin_loss")
    a.position=Vector3.ZERO;b.position=Vector3(1,0,0);_collisions()
    assert(a.position.distance_to(b.position)>2.19,"Hull contacts must separate boats")
    print("MECHANIC_PASS hull_collision")
    a.speed=0;a.controls=Vector3.ZERO;a.position=world.samples[0]+Vector3(0,3,0)
    a.vertical_velocity=0;a.hull_pitch=0;a.hull_roll=0;a.pitch_velocity=0;a.roll_velocity=0
    for i in range(20):a.simulate_buoyancy(1.0/60,0,world.wave_scale)
    assert(a.position.y<2.7,"Gravity must lower a hull raised above the water")
    for i in range(480):a.simulate_buoyancy(1.0/60,0,world.wave_scale)
    var stable_y=a.position.y
    assert(absf(a.vertical_velocity)<0.05,"Buoyancy must settle into equilibrium")
    for i in range(180):a.simulate_buoyancy(1.0/60,float(i)/60,world.wave_scale)
    assert(absf(a.position.y-stable_y)>0.02,"Animated waves must physically move the hull")
    print("MECHANIC_PASS four_point_buoyancy_gravity_wave_response")
    a.put_on_grid(a_pos,a.heading);b.put_on_grid(b_pos,b.heading)
    begin_race(selected_map,race_entries)

func _test_tick(_dt:float)->void:
    if not sequence_dir.is_empty() and frame_clock>8 and frame_clock<12 and frame_clock-sequence_time>.125:
        sequence_time=frame_clock;sequence_frame+=1
        _capture_frame.call_deferred(sequence_dir+"/%04d.png"%sequence_frame)
    if "--host" in test_args and mode=="lobby" and players.size()>=2 and frame_clock>3:_start()
    if not screenshot_path.is_empty() and not screenshot_taken and frame_clock>screenshot_time:
        screenshot_taken=true
        _capture.call_deferred()
    if frame_clock>test_end:
        var report={"mode":mode,"map":selected_map,"networked":networked,"players":players.size(),"boats":boats.size(),"elapsed":elapsed,"local_id":local_id,"fps":Engine.get_frames_per_second(),"racers":[]}
        for b in boats.values():report.racers.append({"id":b.peer_id,"position":[b.position.x,b.position.y,b.position.z],"pitch":b.hull_pitch,"roll":b.hull_roll,"lap":b.lap,"checkpoint":b.checkpoint,"coins":b.coins,"item":b.held_item,"finished":b.finished,"finish_time":b.finish_time,"speed":b.speed})
        var file=FileAccess.open("user://test_"+("host" if "--host" in test_args else "client" if "--client" in test_args else "solo")+"_map"+str(selected_map)+".json",FileAccess.WRITE)
        if file:file.store_string(JSON.stringify(report,"  "))
        print("BENCHY_TEST_RESULT "+JSON.stringify(report))
        _quit_game()

func _capture()->void:
    await RenderingServer.frame_post_draw
    var image=get_viewport().get_texture().get_image()
    var error=image.save_png(screenshot_path)
    print("BENCHY_SCREENSHOT ",screenshot_path," ",error)

func _capture_frame(path:String)->void:
    await RenderingServer.frame_post_draw
    var frame=get_viewport().get_texture().get_image();frame.resize(960,600,Image.INTERPOLATE_BILINEAR);frame.save_png(path)
