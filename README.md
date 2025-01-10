# **Pengine**
A simple 3D game engine written in C++ and Vulkan.

## **Platforms:**
* Windows
* Linux

## **How to build**
### **Windows**
* Download GLFW 64-bit Windows binaries from https://www.glfw.org/download.html.
* Download Vulkan SDK from https://vulkan.lunarg.com.
* Set up next environment variables: `GLFW_INCLUDE`, `GLFW_PATH_LIB`, `SPIRV_REFLECT`, `VULKAN_INCLUDE`, `VULKAN_LIB`.\
For example how I set it:\
`GLFW_INCLUDE = C:/GLFW/Includes`\
`GLFW_PATH_LIB = C:/GLFW/Lib`\
`VULKAN_INCLUDE = C:/VulkanSDK/1.3.268.0/Include`\
`VULKAN_LIB = C:/VulkanSDK/1.3.268.0/Lib`\
`SPIRV_REFLECT = C:/VulkanSDK/1.3.268.0/Source`
* Clone and run CMake.
```
git clone --recursive -b main https://github.com/Shturm0weak/Pengine-2.0.git
```
```
cd Pengine-2.0 && mkdir Build && cd Build && cmake ..
```
* Open Pengine (for VS 20XX `Pengine-2.0/Build/Pengine.sln`) and set up `SandBox` as a startup project.
* Try to build and run.

### **Linux**
* If there is no GLFW, so download GLFW for your linux distribution.
* Download Vulkan SDK from https://vulkan.lunarg.com.
* Set up next environment variables: `SPIRV_REFLECT`, `VULKAN_INCLUDE`, `VULKAN_LIB`.\
For example how I set it:\
`VULKAN_INCLUDE = /home/alexander/Vulkan/1.3.275.0/x86_64/include`\
`VULKAN_LIB = /home/alexander/Vulkan/1.3.275.0/x86_64/lib64`\
`SPIRV_REFLECT = /home/alexander/Vulkan/1.3.275.0/x86_64/include`
* Clone and run CMake.
```
git clone --recursive -b main https://github.com/Shturm0weak/Pengine-2.0.git
```
```
cd Pengine-2.0 && mkdir Build && cd Build && cmake ..
```
* Open Pengine and set up `SandBox` as a startup project.
* Try to build and run.

## **Graphics Features**
* Deferred Instanced Rendering
* Basic PBR
* Directional Lights
* Point Lights
* Normal Maps
* Cascade Shadow Maps
* SSAO
* SSR
* Bloom
* FXAA
* Atmospheric Scattering
* Forward Order Dependent Transparency Rendering
* Multiple Viewport Rendering
* Assync Resource Loader
* Scriptable Materials

## **Used Libraries**
* GLFW
* GLM
* Vulkan
* Entt
* Assimp
* ImGui
* Yaml-cpp

## **Screenshots**
![image](https://github.com/user-attachments/assets/f1cff851-3218-4c87-ab61-abc023539da0)
![image](https://github.com/user-attachments/assets/9f742be0-1fba-4466-9e50-87294011cba3)
![image](https://github.com/user-attachments/assets/eb2f3c0e-3a5d-4883-8825-3ad99b871cf8)
![image](https://github.com/user-attachments/assets/3dbf3b3d-84ae-4129-ac96-dfd68cf8fed6)
![image](https://github.com/user-attachments/assets/ddfa34bb-5934-411c-b84f-588e5e4bd7f0)
![image](https://github.com/user-attachments/assets/053e7830-4217-4a9d-a578-dc31a80d86fc)
![image](https://github.com/user-attachments/assets/2c69fa6d-59a7-4ea6-a756-6ab668ef6704)
![image](https://github.com/user-attachments/assets/19903bde-e2ba-4439-acfb-994f382ca625)
<img src="https://github.com/user-attachments/assets/a43369a9-151e-467e-82db-7f0dc3a3f1a7">
