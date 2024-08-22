# **Pengine**
A simple 3D game engine.

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
`VULKAN_LIB = C:/VulkanSDK/1.3.268.0/Lib`
* Clone and run CMake.
```
git clone --recursive -b main https://github.com/Shturm0weak/Pengine-2.0.git
```
```
cd Pengine-2.0 && mkdir Build && cd Build && cmake ..
```
* Open `Pengine-2.0/Build/Pengine.sln` and set up `SandBox` as a startup project.
* Try to build and run.

## **Screenshots**
![image](https://github.com/user-attachments/assets/a5c01499-cf92-4e7b-8a19-e8e2a73d1be0)
![image](https://github.com/user-attachments/assets/2380599f-25ae-437c-baef-b907dfb2b8fe)

