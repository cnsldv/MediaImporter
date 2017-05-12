# Vita Media Importer
A simple homebrew for importing media into the PS Vita Video player. It removes the need to import your media into CMA/QCMA on your computer.

## What it does
* adds mp4 files found in ux0:/video and subdirectories to the video player database
* removes from database videos whose files cannot be found
* sets icons if found with same filename & extension .THM, .thm, .JPG or .jpg

## What it doesn't do
* add files under uma0: for some reason the video player says "file type not supported" (uma0: mapped as ux0: works)
* check whether the media is actually playable
* create thumbnail (this is done by video app when you start it)
* set video size metadata (maybe later)
* import photos & videos into the camera app (maybe later)
* import music into the music app (maybe later)

## Instructions
Download and install the vpk. Place .mp4 files in ux0:/video and run it whenever you have added or removed media. When mounting USB as ux0: on VitaTV you might need to run every time you remount, nevermind it only takes a minute. Press cross to update the database or triangle to delete all media from the database (not the files). Now go to the video player to play your videos.

## Build Instructions
Requires Vita SDK and cmake. cd to the top level dir and run

* mkdir build
* cd build
* cmake ..
* make

## Thanks
Media Importer is highly based on the Henkaku offline installer. Thanks to Team Molecule for Henkaku and the offline installer and for open-sourcing to help others produce homebrew more easily.
