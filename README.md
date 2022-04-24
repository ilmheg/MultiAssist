![mabanner](gh/banner.png)
# MultiAssist
<p>
<img align="right" width="300" src="https://user-images.githubusercontent.com/103583531/164975695-d08aa30d-5bb8-403b-b4bb-927dadeb906a.png" />
Upon opening MultiAssist, you'll see the MultiAssist GUI with the "Extract" tab selected. You can resize this window however you like, however it was designed for use at its default size.
</p>

[AnimAssist](https://github.com/lmcintyre/AnimAssist) GUI with support for repacking .pap files with multiple animations, minor bug fixes, and more extensive export options, including FBX. It can also be used without the GUI through command line arguments. 

This project assists in the editing of FFXIV animation files. 

I started this fork so I had a quick way to extract some animation data from XIV that AnimAssist itself did not provide by default, I expanded upon it until it became MultiAssist. The multi-animation repacking process seems to be an enigma to many due to some unfortunate things, hopefully this makes it more accessible. 

Essentially everything good in this project comes from Perchbird's work, everything bad in this project is probably by me. 


<p align="center">
    <img width=400 src="https://user-images.githubusercontent.com/103583531/164982393-3df386b5-51af-4fc6-adff-85f47e7728c2.gif" width="600" />
    <p align="center"> 
        Au Ra with Crash Bandicoot's run, made with .fbx exports. 
    </p>
</p>

# Getting Started
Visit [the project's wiki](https://github.com/ilmheg/MultiAssist/wiki) for guides on MultiAssist and demonstrations of animation editing for XIV! 

# Prerequisites
### Required software
* The latest release in [Releases](https://github.com/ilmheg/MultiAssist/releases) (or your own build of the project).
* [VC++2012 32-bit Redist](https://www.microsoft.com/en-us/download/details.aspx?id=30679#)
    * Select "VSU_4\vcredist_x86.exe" when choosing which file to download.
    * This is required for Havok stuff, please install it.

### Recommended software
* [FFXIV Explorer (goat fork)](https://github.com/goaaats/ffxiv-explorer-fork)
    * Convenient way to browse and extract Skeleton and Animation files. Please use raw extraction (Ctrl+Shift+E) for MultiAssist purposes. Other methods of extractions, such as TexTools, will also work.
* [TexTools](https://www.ffxiv-textools.net/), [Penumbra](https://github.com/xivdev/Penumbra/), or any other method of importing a .pap file into FFXIV.
* (**Optional, 3DS Max only**) [HavokMax - 3DS Max plugin](https://github.com/PredatorCZ/HavokMax/)
    * HavokMax adds the functionality to import and export .hkx packfiles through 3DS max.
    * You do not need this if you plan to edit .FBX files. 
* A 3D editor
    * To edit the extracted animations.
    * I only tested with 3DS max. 
### Optional Software
* Python 3 (If running from MultiAsisst.py rather than the pre-built release)
    * I was using python 3.10.4
    * BeautifulSoup: `pip install bs4` in terminal/command line.
    * Dear PyGui: `pip install dearpygui`  in terminal/command line.
    * If you are using the .py, make sure to build or otherwise acquire the companion executables.
* [Godbert](https://github.com/xivapi/SaintCoinach#godbert)
    * May be useful in assisting yourself in familiarizing yourself with the location of FFXIV animations, entirely optional.
* [VFXEditor](https://github.com/0ceal0t/Dalamud-VFXEditor)
    * Way to edit the timeline section of animation files, among many other things.

# Installation
To install MultiAssist, head to [Releases](https://github.com/ilmheg/MultiAssist/releases) and download the latest MultiAssist.zip. Extract the files to an accessible location and run MultiAssist.exe to use the GUI. Do not remove any of the extracted files in this folder.

Make sure you have looked at the [Required software](#required-software) and have installed VC++2012 32-bit Redist.


# Note on FBX support:
fbx2havok is an older proof-of-concept project by Perchbird. The build in this project makes a number of changes for ease of use in Blender and prevents re-imported animations from being glitchy. If something goes wrong, keep the unpolished nature of this in mind. I am interested in any errors you encounter using this method. 

In the current release, please use blender, either as a middle-man, or for all FBX editing. 

I am very interested in the progression of this and am working on ironing out remaining issues with FBX exports.


# Technical rundown of the process
### Extraction
#### MultiAssist.py
1. Strips headers of pap and sklb, leaving us with a havok binary tagfile of the animation and skeleton.
#### animassist.exe
2. Creates XML packfile of Skeleton from the havok tagfile.
3. Selects the animation and binding of the specified index and repackages them with the Skeleton packfile into a new havok packfile.
#### MultiAssist.py
4. Output differs depending on file type input.
    * FBX - Runs havok data through havok2fbx.exe and returns an FBX.
    * Packfile - Returns the packfile from step 3.
    * Tagfile - Returns the havok anim data from step 1.
    * XML - Uses animassist.exe and the havok data from step 1 to return an XML file with the same format as Havok Preview Tool exports.

### Repackaging
This is almost completely handled by animmassist.exe and the Havok.sdk now. The old process used conversion to XML and literally merged the Animation object and Binding object with the old file.

Now, the process is similar, just done in one step through the Havok SDK.

### Changes to HAVOK2FBX2HAVOK
* Doesn't set axis orientation
* Exports directly to binary
* Sets customframerate = 30
* Experimental optional fix for certain components not animating when exported (nnumberoftracks = numberofbones)
* Outputs uncompressed animations
    * Predictive compression available through animassist.exe

# Future development plans for MultiAssist
I hope to continue supporting this project. In the near future I will improve input validation and error reporting within the GUI. Additionally, I am looking for ways to further streamline the process in the more distant future, such as exporting directly to penumbra. Less generally, I believe direct animation swapping and features of a similar vein are also within the scope of this project.

# Building, contributions, notes
## Notes
Please note that I nor this project are affiliated with AnimAssist, fbx2havok, havok2fbx, or any other projects, nor their contributors.

In the spirit of the original AnimAssist, this project's license is WTFPL. Refer to the LICENSE within the release folder for the license of the included redistributed executables.

I hope this program further streamlines animation editing in FFXIV and makes the process more accessible. 

Lastly, please note that I *really* don't know what I'm doing!!

## Building
Building animassist.exe requires the Havok 2014 SDK and an env var of HAVOK_SDK_ROOT set to the directory, as well as the Visual C++ Platform Toolset v110. This is included in any install of VS2012, including the Community edition.

You can find the Havok SDK to compile with [in the description of this video](https://www.youtube.com/watch?v=U88C9K-mSHs). Please note that is NOT a download controlled by any contributor to AnimAssist, use at your own risk.

Building the associated fbx2havok/havok2fbx projects will additionally require FBX SDK 2014.2.1. Refer to the respective repositories for further guidance.

For the python pre-requisites, see [Optional Software](#optional-software).

## Contributing
Contributions are welcome, please help make bad things good etc 
