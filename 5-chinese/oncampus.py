#This module is intended to be run on a linux machine on the Warwick campus
#it provides two boolean variables:
# oncampus : whether you are sitting on campus (which implies you probably have a very fast connection to this machine)
# draw : whetheer it would be a good idea to try to plot graphs to the screen. This will be false if you are offcampus or are using SSH without X forwarding
import os, subprocess, string

if os.name=="posix":
    if "STY" in os.environ or "TMUX" in os.environ: #we are inside gnu screen or tmux
        #It's not obvious what behaviour is correct in this case - same session can be used in multiple places.
        #Simplest is to treat this as off campus.
        #When on campus just do graphing in another terminal.
        oncampus = False
    elif "SSH_CLIENT" in os.environ:
        oncampus = -1 < subprocess.check_output(["who", "-m"]).find(b"warwick")
    else:
        oncampus = True
        
    draw = oncampus and "DISPLAY" in os.environ
else:
    #You are probably sitting at your machine
    draw = True
    oncampus = True
