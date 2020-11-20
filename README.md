# VulkanDemos

This project contains several small demos using the Vulkan API. There are several "bad" things going on, for example
only one image can be "in flight" at a time. That means that pipeline stages which have already completed are not
able to start work on a new image. Thus, the code should be taken with a grain of salt as I was learning (and still do)
Vulkan during its creation :)


[Cubemapping Demo](https://github.com/michaeleggers/VulkanDemos/blob/main/A4_Cubemapping/main.cpp)

![alt text](https://github.com/michaeleggers/VulkanDemos/blob/main/readme_assets/cubemapping_gh_readme.gif "Cubemapping")

[Normal Mapping Demo](https://github.com/michaeleggers/VulkanDemos/blob/main/bump-mapping-demo/main.cpp)
![alt text](https://github.com/michaeleggers/VulkanDemos/blob/main/readme_assets/nm_gh_readme.gif "Normal Mapping")

[Soft Shadows using PCF + PCSS](https://github.com/michaeleggers/VulkanDemos/blob/main/A5_PCSS/main.cpp)
![alt text](https://github.com/michaeleggers/VulkanDemos/blob/main/readme_assets/pcss_gh_readme.gif "PCSS")

[Soft Shadows using Nvidia's RTX extensions for hw-accelerated raytracing](https://github.com/michaeleggers/VulkanDemos/blob/main/rtx-raytracing/main.cpp)
![alt text](https://github.com/michaeleggers/VulkanDemos/blob/main/readme_assets/rtx_gh_readme.gif "RTX Soft Shadows")