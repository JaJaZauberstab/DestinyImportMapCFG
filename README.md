# DestinyImportMapCFG
Allows importing Charm(*.CFG) Maps into UE5.4.4

**DISCLAIMERS:**
- Lighting is imported using Transform, Cookie and Colour. They are also imported by type, whether they cast shadows or not. Important to Note: I Know very little about Lighting in Unreal Engine, let alone Destiny
- I have very little understanding of Destiny's Material and Rendering Pipeline, hence why I only automatically add TextureSample Nodes and Diffuse maps to materials currently. I am very open to insights should anyone have any to share :)
- SkeletalMesh Materials may need to be manually reassigned to the ones in .../[MapNameFolder]/Materials/
- Map Scale is set to 100.0 by default this is to ensure the map is actually a normal size. The fbx files exported by Charm are quite small, I assume is related to the S&box support maybe. Scale can be changed easily in the plugin UI

**Features:**
- Uses data stored in Charm Exported *.cfg files to rebuild Maps from Destiny 1 and Destiny 2 in Unreal Engine 5.4.4
- Imports all required Textures and adds them as sample within the relevant materials, first sRGb texture referenced in every material is assigned as the Base Colour/Diffuse Map
- Map decorators such as grass, rocks foliage etc; are spawned into the map High Instance Static Models (HISM) to improve map preformance

**Unsupported/Future Features:**
- Atmosphere is not imported at this time
- Decals are not generated yet
- GlobalChannels cos idk wha they are
- Real Materials instead of just ones with diffuse
- More Accurate Lighting

The Dungeons (Destiny: The Taken King)
![image](https://github.com/user-attachments/assets/9082d9f7-394d-46e7-b0a9-e6bffea80794)
![image](https://github.com/user-attachments/assets/c0b0da7c-89bd-43c6-9744-28589054d72c)

The Farm (Destiny 2: The Red War)

