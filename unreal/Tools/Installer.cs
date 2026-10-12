using System;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Diagnostics;
using System.Drawing;
using System.Windows.Forms;
using System.ComponentModel;
using Microsoft.Win32;

[assembly: AssemblyTitle("Benchy Racers Unreal Setup")]
[assembly: AssemblyProduct("Benchy Racers Unreal")]
[assembly: AssemblyVersion("1.5.1.0")]

class Setup : Form {
    static readonly string InstallDir=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Programs","Benchy Racers Unreal");
    static readonly string StartDir=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Programs),"Benchy Racers Unreal");
    static readonly string RegPath=@"Software\Microsoft\Windows\CurrentVersion\Uninstall\BenchyRacersUnreal";
    Label status=new Label(); Button action=new Button(); Button cancel=new Button();
    CheckBox desktop=new CheckBox();CheckBox launch=new CheckBox();ProgressBar progress=new ProgressBar();
    bool removal=false;bool installed=false;

    [STAThread] static void Main(string[] args) {
        Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
        if(args.Length==2 && args[0]=="--preview") {
            using(Setup view=new Setup(false)) {
                view.ShowInTaskbar=false;view.Opacity=0;view.Show();Application.DoEvents();
                IntPtr formHandle=view.Handle;
                foreach(Control child in view.Controls){IntPtr childHandle=child.Handle;}
                view.PerformLayout();
                using(Bitmap image=new Bitmap(view.Width,view.Height)) {view.DrawToBitmap(image,new Rectangle(0,0,view.Width,view.Height));image.Save(args[1],System.Drawing.Imaging.ImageFormat.Png);}
                view.Hide();
            }
            return;
        }
        if(args.Length==2 && args[0]=="--verify") {
            using(Stream stream=Assembly.GetExecutingAssembly().GetManifestResourceStream("GamePayload"))
            using(ZipArchive zip=new ZipArchive(stream,ZipArchiveMode.Read))
            using(StreamWriter report=new StreamWriter(args[1])) {
                foreach(ZipArchiveEntry entry in zip.Entries) {
                    using(Stream payload=entry.Open())using(var sha=System.Security.Cryptography.SHA256.Create()){
                        report.WriteLine(entry.FullName+"\t"+entry.Length+"\t"+BitConverter.ToString(sha.ComputeHash(payload)).Replace("-","").ToLowerInvariant());
                    }
                }
            }
            return;
        }
        if(args.Length>0 && args[0]=="--remove") {
            System.Threading.Thread.Sleep(1500);
            try { Remove(); MessageBox.Show("Benchy Racers Unreal wurde entfernt. Deine Einstellungen bleiben erhalten.","Benchy Racers Unreal",MessageBoxButtons.OK,MessageBoxIcon.Information); }
            catch(Exception e) { MessageBox.Show(e.Message,"Entfernen fehlgeschlagen",MessageBoxButtons.OK,MessageBoxIcon.Error); }
            return;
        }
        bool uninstall=false;
        #if UNINSTALLER
        uninstall=true;
        #endif
        if(args.Length>0 && args[0]=="--uninstall")uninstall=true;
        Application.Run(new Setup(uninstall));
    }
    Setup(bool uninstall) {
        removal=uninstall;Text="Benchy Racers Unreal · "+(uninstall?"Deinstallieren":"Installation");
        ClientSize=new Size(680,415);FormBorderStyle=FormBorderStyle.FixedDialog;MaximizeBox=false;StartPosition=FormStartPosition.CenterScreen;
        BackColor=Color.FromArgb(9,24,35);ForeColor=Color.FromArgb(237,248,251);Font=new Font("Segoe UI",11);
        try { Icon=Icon.ExtractAssociatedIcon(Assembly.GetExecutingAssembly().Location); }catch {}
        AddLabel("BENCHY RACERS",new Rectangle(34,28,600,48),27,Color.FromArgb(103,230,209));
        AddLabel(uninstall?"Das Spiel entfernen":"Dein Benchy wartet auf die Startlinie.",new Rectangle(36,86,605,40),17,ForeColor);
        AddLabel(uninstall?"Entfernt Spiel und Verknüpfungen. Persönliche Einstellungen bleiben erhalten.":"Version 1.5.1 · Neon Gelb · Lobby-Adressen · sieben Filamente",new Rectangle(36,138,605,52),11,Color.FromArgb(154,181,195));
        AddLabel("Spielordner",new Rectangle(36,192,600,25),10,Color.FromArgb(154,181,195));
        AddLabel(InstallDir,new Rectangle(36,222,600,40),10,ForeColor);
        desktop.Text="Verknüpfung auf dem Desktop";desktop.Checked=true;desktop.SetBounds(36,270,300,28);Controls.Add(desktop);
        launch.Text="Nach der Installation starten";launch.Checked=true;launch.SetBounds(355,270,300,28);Controls.Add(launch);
        desktop.Visible=launch.Visible=!uninstall;
        progress.SetBounds(36,307,607,8);progress.Visible=false;Controls.Add(progress);
        status.Text=uninstall?"Bitte das Spiel vorher schließen.":"Installation für diesen Benutzer · keine Adminrechte nötig";
        status.ForeColor=Color.FromArgb(154,181,195);status.SetBounds(36,321,607,28);status.Font=new Font("Segoe UI",9);Controls.Add(status);
        action.Text=uninstall?"Deinstallieren":"Installieren";action.SetBounds(385,359,155,38);StyleButton(action,true);Controls.Add(action);
        cancel.Text="Abbrechen";cancel.SetBounds(550,359,95,38);StyleButton(cancel,false);cancel.Click+=(s,e)=>Close();Controls.Add(cancel);
        action.Click+=Begin;FormClosing+=(s,e)=>{if(!action.Enabled && !installed)e.Cancel=true;};
    }
    void AddLabel(string text,Rectangle rect,int size,Color color) {
        Label l=new Label();l.Text=text;l.Bounds=rect;l.Font=new Font("Segoe UI",size,size>=17?FontStyle.Bold:FontStyle.Regular);l.ForeColor=color;Controls.Add(l);
    }
    void StyleButton(Button b,bool primary) {
        b.FlatStyle=FlatStyle.Flat;b.FlatAppearance.BorderSize=0;b.BackColor=primary?Color.FromArgb(255,172,101):Color.FromArgb(24,50,62);
        b.ForeColor=primary?Color.FromArgb(9,24,35):ForeColor;b.Cursor=Cursors.Hand;
    }
    void Begin(object sender,EventArgs evt) {
        if(installed){Close();return;}
        if((Process.GetProcessesByName("BenchyRacers").Length>0 || Process.GetProcessesByName("BenchyRacers-Win64-Development").Length>0)){MessageBox.Show("Bitte Benchy Racers Unreal zuerst schließen.",Text);return;}
        if(removal) {
            string temp=Path.Combine(Path.GetTempPath(),"BenchyRacers-Uninstall-"+Guid.NewGuid().ToString("N")+".exe");
            File.Copy(Assembly.GetExecutingAssembly().Location,temp);
            Process.Start(new ProcessStartInfo(temp,"--remove"){UseShellExecute=true});Close();return;
        }
        bool makeDesktop=desktop.Checked;bool run=launch.Checked;
        action.Enabled=cancel.Enabled=desktop.Enabled=launch.Enabled=false;progress.Visible=true;status.Text="Spieldateien werden installiert …";
        BackgroundWorker worker=new BackgroundWorker();worker.WorkerReportsProgress=true;
        worker.DoWork+=(s,e)=>{
            Directory.CreateDirectory(InstallDir);
            using(Stream stream=Assembly.GetExecutingAssembly().GetManifestResourceStream("GamePayload")) {
                if(stream==null)throw new Exception("Das Spielpaket fehlt.");
                using(ZipArchive zip=new ZipArchive(stream,ZipArchiveMode.Read)) {
                    int i=0;
                    foreach(ZipArchiveEntry entry in zip.Entries) {
                        string destination=Path.GetFullPath(Path.Combine(InstallDir,entry.FullName));
                        string prefix=Path.GetFullPath(InstallDir)+Path.DirectorySeparatorChar;
                        if(!destination.StartsWith(prefix,StringComparison.OrdinalIgnoreCase))throw new Exception("Ungültiger Paketpfad.");
                        if(String.IsNullOrEmpty(entry.Name)){Directory.CreateDirectory(destination);continue;}
                        Directory.CreateDirectory(Path.GetDirectoryName(destination));
                        using(Stream src=entry.Open())using(FileStream dest=File.Create(destination)){src.CopyTo(dest);}
                        worker.ReportProgress(++i*85/zip.Entries.Count);
                    }
                }
            }
            Directory.CreateDirectory(StartDir);
            Shortcut(Path.Combine(StartDir,"Benchy Racers Unreal.lnk"),Path.Combine(InstallDir,"BenchyRacers.exe"),"");
            Shortcut(Path.Combine(StartDir,"Benchy Racers Unreal deinstallieren.lnk"),Path.Combine(InstallDir,"Uninstall.exe"),"");
            if(makeDesktop)Shortcut(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory),"Benchy Racers Unreal.lnk"),Path.Combine(InstallDir,"BenchyRacers.exe"),"");
            using(RegistryKey key=Registry.CurrentUser.CreateSubKey(RegPath)){
                key.SetValue("DisplayName","Benchy Racers Unreal");key.SetValue("DisplayVersion","1.5.1-unreal");
                key.SetValue("InstallLocation",InstallDir);key.SetValue("DisplayIcon",Path.Combine(InstallDir,"BenchyRacers.exe"));
                key.SetValue("UninstallString","\""+Path.Combine(InstallDir,"Uninstall.exe")+"\"");
                key.SetValue("NoModify",1,RegistryValueKind.DWord);key.SetValue("NoRepair",1,RegistryValueKind.DWord);
                long installedBytes=0;
                foreach(string file in Directory.GetFiles(InstallDir,"*",SearchOption.AllDirectories))installedBytes+=new FileInfo(file).Length;
                key.SetValue("EstimatedSize",(int)(installedBytes/1024),RegistryValueKind.DWord);
            }
            worker.ReportProgress(100);
        };
        worker.ProgressChanged+=(s,e)=>progress.Value=e.ProgressPercentage;
        worker.RunWorkerCompleted+=(s,e)=>{
            action.Enabled=cancel.Enabled=true;
            if(e.Error!=null){status.Text="Installation fehlgeschlagen.";MessageBox.Show(e.Error.Message,Text,MessageBoxButtons.OK,MessageBoxIcon.Error);action.Text="Erneut versuchen";return;}
            installed=true;status.Text="Fertig! Benchy Racers Unreal ist im Startmenü.";action.Text="Schließen";cancel.Visible=false;
            if(run){Process.Start(new ProcessStartInfo(Path.Combine(InstallDir,"BenchyRacers.exe")){UseShellExecute=true,WorkingDirectory=InstallDir});Close();}
        };
        worker.RunWorkerAsync();
    }
    static void Shortcut(string path,string target,string arguments) {
        Type type=Type.GetTypeFromProgID("WScript.Shell");object shell=Activator.CreateInstance(type);
        object link=type.InvokeMember("CreateShortcut",BindingFlags.InvokeMethod,null,shell,new object[]{path});
        Type lt=link.GetType();
        lt.InvokeMember("TargetPath",BindingFlags.SetProperty,null,link,new object[]{target});
        lt.InvokeMember("Arguments",BindingFlags.SetProperty,null,link,new object[]{arguments});
        lt.InvokeMember("WorkingDirectory",BindingFlags.SetProperty,null,link,new object[]{InstallDir});
        lt.InvokeMember("Save",BindingFlags.InvokeMethod,null,link,null);
        System.Runtime.InteropServices.Marshal.FinalReleaseComObject(link);System.Runtime.InteropServices.Marshal.FinalReleaseComObject(shell);
    }
    static void Remove() {
        string expected=Path.GetFullPath(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Programs","Benchy Racers Unreal"));
        if(!String.Equals(Path.GetFullPath(InstallDir),expected,StringComparison.OrdinalIgnoreCase))throw new Exception("Ungültiger Installationsordner.");
        if((Process.GetProcessesByName("BenchyRacers").Length>0 || Process.GetProcessesByName("BenchyRacers-Win64-Development").Length>0))throw new Exception("Benchy Racers Unreal läuft noch. Bitte schließen und erneut deinstallieren.");
        if(Directory.Exists(InstallDir))Directory.Delete(InstallDir,true);
        if(Directory.Exists(StartDir))Directory.Delete(StartDir,true);
        string shortcut=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory),"Benchy Racers Unreal.lnk");
        if(File.Exists(shortcut))File.Delete(shortcut);
        Registry.CurrentUser.DeleteSubKeyTree(RegPath,false);
    }
}
