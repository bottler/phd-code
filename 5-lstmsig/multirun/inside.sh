sudo apt-get update
#sudo apt-get upgrade #no need for the safety of this
sudo apt-get dist-upgrade
sudo apt install python3-pip python3-scipy htop colordiff emacs24-nox libopenblas-dev #x11-apps may be useful, to test x-forwarding, emacs24
#sudo -H pip3 install ipython keras==1.2.2 h5py tabulate matplotlib theano azure-servicebus
sudo -H pip3 install --upgrade ipython keras h5py tabulate matplotlib theano azure-servicebus
#sudo -H pip3 install git+git://github.com/bottler/iisignature.git@master
sudo -H pip3 install --upgrade iisignature

#To run just the above but not the below, e.g. to rerun this script, just run:   JRSKIP=9 source inside.sh
if [[ -z "$JRSKIP" ]]
then
    echo alias em=emacs >> .bashrc
    echo alias rm=\"rm -i\" >> .bashrc
    echo alias mv=\"mv -i\" >> .bashrc
    echo alias cp=\"cp -i\" >> .bashrc
    echo alias r=\'ipython3 -i -c \"import numpy, numpy as np\; import results, results as r\"\' >>.bashrc
    KERAS_BACKEND=theano python3 -c "import keras"
    perl -i -wnpe 's/tensorflow/theano/' .keras/keras.json
fi


#fsharp
#more aliases?



#get list https://askubuntu.com/questions/2389/generating-list-of-manually-installed-packages-and-querying-individual-packages
