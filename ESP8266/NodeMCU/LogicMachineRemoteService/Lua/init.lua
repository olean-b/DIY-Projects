FileToExecute="init_startup.lua"

l = file.list();
for k,v in pairs(l) do
  if k == FileToExecute then
    print("*** You've got 5 sec to stop timer 0 ***")
    print("To abort, send =tmr.stop(0) ")
    tmr.alarm(0, 5000, 0, function()
      print("Executing ".. FileToExecute)
      dofile(FileToExecute)
    end)
  end
end
