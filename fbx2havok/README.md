Changes
=============================
# General
* Removes hkaAssetManagerUtil
    * There was no reason to keep this here.

## tofbx.exe
* Sets a custom frame rate of 30fps
* Exports to binary instead of ASCII
* No longer sets axis orientation
    * This is purely for blender comfort.
* All bones will animate. 
    * Currently investigating potential negative side effects to this.
    * This provides the same behavior as HavokMax would, I think this not being a thing was an oversight.
        * HavokMax does have "Optimize Tracks" to overcome the potential negative side of this, but I don't think I'll implement this.


## fbx2havok.exe
* Exports an uncompressed animation

## TODO
* Optional compression
    * Predictive and Quantized perform well (Predictive more so, on the compression ratio side) (assuming they run in XIV)
    * SplineCompression sometimes works okay.
* Optional System Unit: metres (Settings this makes for less work in Blender, more work in 3DS and so on, no advantage to enabling it universally)
* Removing the loc/rot/scl keyframes from tofbx would be nice. The not bone specific ones. Really only for blender this.










FBX to Havok for FFXIV
=============================

Proof of concept allowing for conversion of Havok animations to FBX and back to Havok, facilitating custom animations in XIV.

## To use
Grab the skeleton and animation you want from XIV and strip the XIV skeleton header, and XIV animation header and the timeline at the end if it exists. In other words, get the Havok data out of the files.
Build and use

```tofbx.exe -hk_skeleton <skeleton_file> -hk_anim <animation_file> -fbx <fbx_output_file>```
 
 to convert the animation and skeleton to FBX. Open the FBX file and fiddle with it however you want.
 
Use
 
```fbx2havok.exe -hk_skeleton <original_skeleton_file> -hk_anim <original_animation_file> -fbx <fbx_input_file> -hkout <havok_output_file>```

to convert the FBX file you edited back into a Havok animation. Import the file into FFXIV and use it, or use the Havok Preview Tool to check the animation. 

# Development
Required Libraries/Applications:
- Havok SDK 2014-1-0
- FBX SDK 2014.2.1
- Visual Studio 2012 for Platform Toolset V110

Required Environment Variables for installed libraries:
- HAVOK_SDK_ROOT
- FBX_SDK_ROOT

# Support
No support is provided. FBX back to Havok conversion is still glitchy and proof-of-concept level, but works. Animation paths in XIV are finicky, there are certain animations that use a different path depending on if you're looking at a character taller/shorter than you. Make sure you're using the correct path when importing. On that note, you must find your own way to import files into XIV. 