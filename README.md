# Vita Media Importer
A simple homebrew for importing media into the PS Vita Video and Muisc players. It removes the need to import your media into CMA/QCMA on your computer.

## What it does
* adds mp4 files found in ux0:/video and subdirectories to the video player database
* adds mp3 files found in ux0:/music and subdirectories to the music player database
* removes from databases items whose files cannot be found
* sets icons for video files if found with same filename & extension .THM, .thm, .JPG or .jpg

## What it doesn't do
* check whether the media is actually playable
* create thumbnail (this is done by video app when you start it)
* set video size metadata (maybe later)
* import photos & videos into the camera app (maybe later)

## Instructions
* download and install the vpk
* place .mp4 files in ux0:/video
* place .mp3 files in ux0:/music
* run MediaImporter and press cross to add them to the databases
* press triangle to remove all items from the databases (leaving files intact)
* go to Video Player to play your videos
* go to Music Player to play your mp3s

## Known Limitations
* music doesn't play at all from USB even when mounted as ux0:
* sometimes (especially on first import) you will get an error trying to play in Music player, reboot fixes and should be OK to import more without rebooting
* when mounting USB as ux0: you will need to rerun MediaImporter to add your videos

The Music player seems to have a background process that causes issues when mounting USB as ux0: VitaShell can play mp3s from USB, maybe that's an option for playing from USB

## Build Instructions
Requires Vita SDK and cmake. cd to the top level dir and run

* mkdir build
* cd build
* cmake ..
* make

## Changelog

### 0.91
* adds video folders using top level folder name, only works on inserting video so clear the database first if you have videos from an earlier version

### 0.9
* adds import of music from ux0:/music
* performance boost
* live update of "Added x videos" string
* fix for leaking file handles

### 0.8
* imports videos from ux0:/video only

## Thanks
Media Importer is highly based on the Henkaku offline installer with ID3 tag code borrowed from VitaShell. Thanks to Team Molecule for Henkaku and the offline installer and for open-sourcing to help others produce homebrew more easily.
