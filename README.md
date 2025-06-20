# DestinyImportMapCFG
Allows importing Charm(*.CFG) Maps into UE5.4.4

**DISCLAIMERS:**
- Lighting is imported using Transform and Colour. They are also imported by type, whether they cast shadows or not. Important to Note: I Know very little about Lighting in Unreal Engine, let alone Destiny's
- Most of the time the GStack will be applied as the diffuse as seen in the Farm and on Mars below. I have very little understanding of Destiny's Material and Rendering Pipeline, hence why I only automatically add TextureSample Nodes and Diffuse maps to materials currently. I am very open to insights should anyone have any to share :)
- Map Scale is set to 100.0 by default this is to ensure the map is actually a normal size. The fbx files exported by Charm are quite small, I assume is related to the S&box and Blender support maybe. Scale can be changed easily in the plugin UI
- Import is going to be very slow for large maps, .cfg files greater than 500kb will take half an hour or more dependant on hardware

**Features:**
- Uses data stored in Charm Exported *.cfg files to rebuild Maps from Destiny 1 and Destiny 2 in Unreal Engine 5.4.4
- Imports all required Textures and adds them as sample within the relevant materials, first sRGb texture referenced in every material is assigned as the Base Colour/Diffuse Map
- Map decorators such as grass, rocks foliage etc; are spawned into the map High Instance Static Models (HISM) to improve map performance

**Unsupported/Future Features:**
- Atmosphere is not imported at this time
- Decals are not generated yet
- GlobalChannels cos idk what they are
- Real Materials instead of just ones with diffuse
- More Accurate Lighting with Cookies/LMF
- Better UI

**The Dungeons (Destiny: The Taken King)**
*Import Images Taken Prior to fixed Y-Axis Map Flipping*
![image](https://github.com/user-attachments/assets/9082d9f7-394d-46e7-b0a9-e6bffea80794)
![image](https://github.com/user-attachments/assets/c0b0da7c-89bd-43c6-9744-28589054d72c)

**The Farm (Destiny 2: The Red War)**
*Import Images Taken Prior to fixed Y-Axis Map Flipping*
![image_2025-06-19_122514630](https://github.com/user-attachments/assets/9cf01a7f-3d6a-4d7a-8d33-18fc0a437a09)
![image_2025-06-19_122555630](https://github.com/user-attachments/assets/9a65d629-e070-43d2-8fab-30112f574381)

**Alton Dynamo, Hellas Basin (Destiny 2: The Warmind)**
*Import Images Below after fix*
![image](https://github.com/user-attachments/assets/d6ae6613-c7a3-4fce-bb43-294966c42ee9)
![image](https://github.com/user-attachments/assets/e8b87c22-f7ea-49e0-8dde-d912a2535b02)
