using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Security.Cryptography;
using System.Threading;
using System.Web.Script.Serialization;
using System.Windows.Forms;

public class UpdateFile { public string path, sha256, oldSha256; public long bytes; }
public class Patch { public string fromVersion, url, sha256; public long bytes; public UpdateFile[] files; }
public class UpdateManifest { public int schema; public string version; public Patch[] patches; }
public static class UpdateCore {
 public static string Hash(string path) { using(var h=SHA256.Create())using(var f=File.OpenRead(path))return BitConverter.ToString(h.ComputeHash(f)).Replace("-","").ToLowerInvariant(); }
 public static string Inside(string root,string relative) {
  if(string.IsNullOrWhiteSpace(relative)||Path.IsPathRooted(relative)||relative.Contains(":")||relative.Contains("\\")||relative.Split('/').Length==0)throw new Exception("Ungueltiger Update-Pfad.");
  foreach(var part in relative.Split('/'))if(part==".."||part=="."||part.Length==0)throw new Exception("Ungueltiger Update-Pfad.");
  if(relative.IndexOf("Saved",StringComparison.OrdinalIgnoreCase)>=0||relative.EndsWith("BenchyEOS.ini",StringComparison.OrdinalIgnoreCase))throw new Exception("Persoenliche Dateien duerfen nicht aktualisiert werden.");
  var full=Path.GetFullPath(Path.Combine(root,relative));var prefix=Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar)+Path.DirectorySeparatorChar;
  if(!full.StartsWith(prefix,StringComparison.OrdinalIgnoreCase))throw new Exception("Pfad ausserhalb des Spiels.");
  for(var p=Path.GetDirectoryName(full);p!=null&&p.Length>=prefix.Length-1;p=Path.GetDirectoryName(p))if(Directory.Exists(p)&&(File.GetAttributes(p)&FileAttributes.ReparsePoint)!=0)throw new Exception("Verknuepfte Ordner werden nicht aktualisiert.");
  if(File.Exists(full)&&(File.GetAttributes(full)&FileAttributes.ReparsePoint)!=0)throw new Exception("Verknuepfte Dateien werden nicht aktualisiert.");
  return full;
 }
 public static void CheckUrl(string url){var u=new Uri(url);if(u.Scheme!="https"||u.Host!="github.com"||!u.AbsolutePath.StartsWith("/Wambo-20/BenchyRacers/releases/download/",StringComparison.Ordinal))throw new Exception("Unbekannte Update-Quelle.");}
 public static void Apply(string root,string zip,Patch patch,Action<string> status,int failAfter=-1) {
  if(Hash(zip)!=patch.sha256||new FileInfo(zip).Length!=patch.bytes)throw new Exception("Der Download ist beschaedigt. Bitte erneut versuchen.");
  var expected=new Dictionary<string,UpdateFile>(StringComparer.OrdinalIgnoreCase);
  foreach(var f in patch.files){Inside(root,f.path);if(expected.ContainsKey(f.path))throw new Exception("Doppelter Update-Pfad.");expected.Add(f.path,f);}
  var stage=Path.Combine(root,".benchy-update-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(stage);
  var moved=new List<string>();var installed=new List<string>();bool rollbackFailed=false;
  try {
   using(var z=ZipFile.OpenRead(zip)) {
    if(z.Entries.Count!=expected.Count)throw new Exception("Falscher Inhalt im Update.");
    var seen=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
    foreach(var e in z.Entries){UpdateFile f;if(!seen.Add(e.FullName)||!expected.TryGetValue(e.FullName,out f)||e.Length!=f.bytes)throw new Exception("Falsche Datei im Update.");var target=Inside(stage,"new/"+f.path);Directory.CreateDirectory(Path.GetDirectoryName(target));e.ExtractToFile(target);if(Hash(target)!=f.sha256)throw new Exception("Pruefsumme stimmt nicht: "+f.path);}
   }
   foreach(var f in patch.files){var dst=Inside(root,f.path);if(f.oldSha256!=null){if(!File.Exists(dst)||Hash(dst)!=f.oldSha256)throw new Exception("Die installierte Version ist veraendert: "+f.path+". Bitte den vollstaendigen Installer verwenden.");}else if(File.Exists(dst))throw new Exception("Eine neue Datei existiert bereits: "+f.path);}
   foreach(var f in patch.files){var dst=Inside(root,f.path);var backup=Inside(stage,"backup/"+f.path);status("Installiere "+f.path);Directory.CreateDirectory(Path.GetDirectoryName(dst));if(File.Exists(dst)){Directory.CreateDirectory(Path.GetDirectoryName(backup));File.Move(dst,backup);moved.Add(f.path);}File.Move(Inside(stage,"new/"+f.path),dst);installed.Add(f.path);if(failAfter==installed.Count)throw new IOException("Test: Installationsfehler");}
  }catch(Exception original){
   try {for(int i=installed.Count-1;i>=0;i--)File.Delete(Inside(root,installed[i]));for(int i=moved.Count-1;i>=0;i--)File.Move(Inside(stage,"backup/"+moved[i]),Inside(root,moved[i]));}catch(Exception restoration){rollbackFailed=true;throw new Exception("Wiederherstellung fehlgeschlagen. Sicherung behalten unter "+stage+". "+restoration.Message,original);}throw;
  }finally{if(!rollbackFailed&&Directory.Exists(stage))Directory.Delete(stage,true);}
 }
}
public class Updater : Form {
 Label label=new Label(); ProgressBar progress=new ProgressBar(); string root; int parent; bool restart; bool busy;
 const string Feed="https://github.com/Wambo-20/BenchyRacers/releases/latest/download/UpdateManifest.json";
 void Status(string s){if(!IsDisposed)BeginInvoke((Action)(()=>label.Text=s));}
 public Updater(string dir,int pid,bool start){root=Path.GetFullPath(dir);parent=pid;restart=start;Text="Benchy Racers aktualisieren";Width=560;Height=180;StartPosition=FormStartPosition.CenterScreen;label.SetBounds(20,20,500,65);label.Text="Update wird vorbereitet ...";progress.SetBounds(20,100,500,20);progress.Style=ProgressBarStyle.Marquee;Controls.Add(label);Controls.Add(progress);FormClosing+=(s,e)=>{if(busy)e.Cancel=true;};Shown+=(s,e)=>new Thread(Run){IsBackground=true}.Start();}
 void Run(){busy=true;string download=null;try {
  if(!File.Exists(Path.Combine(root,"BenchyRacers.exe"))||!Directory.Exists(Path.Combine(root,"BenchyRacers/Content/Paks")))throw new Exception("Bitte den Ordner mit BenchyRacers.exe waehlen.");
  Status("Warte, bis Benchy Racers beendet ist ...");if(parent>0){try{using(var p=Process.GetProcessById(parent))if(!p.WaitForExit(120000))throw new Exception("Das Spiel wurde nicht beendet.");}catch(ArgumentException){}}
  foreach(var n in new[]{"BenchyRacers","BenchyRacers-Win64-Development"})foreach(var p in Process.GetProcessesByName(n))using(p)throw new Exception("Bitte alle laufenden Benchy-Spiele schliessen.");
  ServicePointManager.SecurityProtocol=SecurityProtocolType.Tls12;
  UpdateManifest m;using(var w=new WebClient()){w.Headers[HttpRequestHeader.UserAgent]="BenchyRacers-Updater/1";m=new JavaScriptSerializer{MaxJsonLength=8*1024*1024}.Deserialize<UpdateManifest>(w.DownloadString(Feed));}
  if(m.schema!=1||m.patches==null||m.patches.Length>30)throw new Exception("Dieses Update braucht einen neueren Installer.");
  var statusPath=Path.Combine(root,"BuildStatus.json");if(!File.Exists(statusPath))throw new Exception("Die Spielversion kann nicht erkannt werden.");
  var state=new JavaScriptSerializer().Deserialize<Dictionary<string,object>>(File.ReadAllText(statusPath));var version=Convert.ToString(state["version"]);
  if(version!=m.version){Patch selected=null;foreach(var p in m.patches)if(p.fromVersion==version)selected=p;if(selected==null)throw new Exception("Fuer Version "+version+" gibt es kein Teilupdate. Bitte den aktuellen Installer herunterladen.");UpdateCore.CheckUrl(selected.url);
   Status("Lade Update "+m.version+" ("+(selected.bytes/1048576)+" MB) ...");download=Path.Combine(Path.GetTempPath(),"BenchyUpdate-"+Guid.NewGuid().ToString("N")+".zip");
   using(var w=new WebClient()){w.Headers[HttpRequestHeader.UserAgent]="BenchyRacers-Updater/1";w.DownloadFile(selected.url,download);}Status("Pruefe Dateien und installiere Update ...");UpdateCore.Apply(root,download,selected,Status);if(!File.ReadAllText(statusPath).Contains(m.version))throw new Exception("Versionspruefung fehlgeschlagen.");
  }
  Status("Benchy Racers ist aktuell. Starte Spiel ...");if(restart)Process.Start(new ProcessStartInfo(Path.Combine(root,"BenchyRacers.exe")){WorkingDirectory=root});busy=false;BeginInvoke((Action)Close);
 }catch(Exception e){busy=false;Status(e.Message+"\nDeine Logos und Einstellungen bleiben erhalten.");BeginInvoke((Action)(()=>progress.Style=ProgressBarStyle.Blocks));}finally{if(download!=null&&File.Exists(download))File.Delete(download);}}
 [STAThread] public static void Main(string[] args){Application.EnableVisualStyles();string root=Path.GetDirectoryName(Application.ExecutablePath);int pid=0;bool restart=true;for(int i=0;i<args.Length;i++){if(args[i]=="--root"&&i+1<args.Length)root=args[++i];else if(args[i]=="--wait-pid"&&i+1<args.Length)pid=int.Parse(args[++i]);else if(args[i]=="--no-restart")restart=false;}
  if(!File.Exists(Path.Combine(root,"BenchyRacers.exe"))){root=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Programs/Benchy Racers Unreal Test");if(!File.Exists(Path.Combine(root,"BenchyRacers.exe"))){using(var f=new FolderBrowserDialog()){f.Description="Ordner mit BenchyRacers.exe waehlen";if(f.ShowDialog()!=DialogResult.OK)return;root=f.SelectedPath;}}}
  Application.Run(new Updater(root,pid,restart));}
}
