using System;using System.IO;using System.IO.Compression;
class UpdaterTests {
 static void Require(bool ok,string name){if(!ok)throw new Exception(name);Console.WriteLine("PASS "+name);}
 static void Main(){var dir=Path.GetFullPath("work/updater-fixture-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(dir);var root=Path.Combine(dir,"game");Directory.CreateDirectory(root);var old=Path.Combine(root,"game.bin");File.WriteAllText(old,"old content");var oldHash=UpdateCore.Hash(old);var fresh=Path.Combine(dir,"fresh.bin");File.WriteAllText(fresh,"new content");var zip=Path.Combine(dir,"patch.zip");using(var z=ZipFile.Open(zip,ZipArchiveMode.Create))z.CreateEntryFromFile(fresh,"game.bin");var p=new Patch{sha256=UpdateCore.Hash(zip),bytes=new FileInfo(zip).Length,files=new[]{new UpdateFile{path="game.bin",sha256=UpdateCore.Hash(fresh),oldSha256=oldHash,bytes=new FileInfo(fresh).Length}}};
  bool failed=false;try{UpdateCore.Apply(root,zip,p,s=>{},1);}catch(IOException){failed=true;}Require(failed&&UpdateCore.Hash(old)==oldHash,"rollback restores previous file");
  File.WriteAllText(old,"changed install");failed=false;try{UpdateCore.Apply(root,zip,p,s=>{});}catch(Exception){failed=true;}Require(failed&&File.ReadAllText(old)=="changed install","wrong base preserved");File.WriteAllText(old,"old content");
  p.sha256=new string('0',64);failed=false;try{UpdateCore.Apply(root,zip,p,s=>{});}catch(Exception){failed=true;}Require(failed&&UpdateCore.Hash(old)==oldHash,"corrupt download rejected");p.sha256=UpdateCore.Hash(zip);
  foreach(var path in new[]{"../outside.bin","/outside.bin","BenchyRacers/Config/BenchyEOS.ini","Saved/design.png"}){failed=false;try{UpdateCore.Inside(root,path);}catch(Exception){failed=true;}Require(failed,"path/private protection "+path);}
  failed=false;try{UpdateCore.CheckUrl("https://example.com/update.zip");}catch(Exception){failed=true;}Require(failed,"unknown host rejected");
  UpdateCore.Apply(root,zip,p,s=>{});Require(UpdateCore.Hash(old)==UpdateCore.Hash(fresh),"verified update installed");Require(Directory.GetDirectories(root).Length==0,"temporary data cleaned");Console.WriteLine("All updater tests passed. Fixture: "+dir);
 }
}
