# Multi-space based subsurface scattering (MSSS)
The implementation of my paper "**[A new rendering algorithm based on multi-space for living soft tissue](https://www.sciencedirect.com/science/article/pii/S0097849321001187)**" published in "Computers & Graphics" in 2021. The implementation code is written by C++  and OpenGL.

> Guo, Guanhui, Yanni Zou, and Peter X. Liu. "A new rendering algorithm based on multi-space for living soft tissue." *Computers & Graphics* 98 (2021): 242-254.(CCF-C, SCI-2)

##  Introduction

To achieve a high degree of visual realism, we propose a new subsurface scattering method, which is based on screen-space and texture-space and an improved two-layer surface reflection model, for living soft tissue rendering. 

The method can be described as a two-step strategy. First, the subsurface scattering in the screen space is computed. The biggest difference from traditional methods is that the back light information stored in the irradiance map of the texture space and the diffusion profile in the dipole model are used to calculate the subsurface scattering distribution formed by the light from the back of the object, which can render more rich details. Second, considering that the living soft tissues are usually covered with a thin layer of mucus, a mucus texture is mapped to the soft tissues as the second layer texture and an improved two-layer surface reflection model is proposed to render the mucus. Moreover, to show the viscosity and smoothness of the mucus, the calculation of highlight component is used in our model, which further enhances the reality of living soft tissue.

![](https://ars.els-cdn.com/content/image/1-s2.0-S0097849321001187-gr4.jpg)

The backlight information stored in the irradiance map of the texture space is considered in calculating the subsurface scattering distribution to improve the rendering quality of soft tissue in the screen space.

![](https://ars.els-cdn.com/content/image/1-s2.0-S0097849321001187-gr3.jpg)

The living soft tissues are usually covered with a thin layer of mucus, to obtain a high fidelity rendering effect, we creatively added a layer of mucus texture to the soft tissue, and an improved two-layer surface reflection model was proposed to render the mucus to show the fresh and living character of the soft tissue.

![](https://ars.els-cdn.com/content/image/1-s2.0-S0097849321001187-gr5.jpg)

## Results
The proposed method was implemented in the rendering of various organs. To demonstrate the performance of the algorithm, the validity and computation efficiency were tested, and the rendering effects were also compared with other methods.

### Subsurface Scattering

Soft tissues rendering with subsurface scattering. heart(first row), liver (second row), lungs (third row).

![](https://ars.els-cdn.com/content/image/1-s2.0-S0097849321001187-gr6.jpg)

### Mucus with Specular Highlight

Comparison of the realistic rendering of living soft tissues: lungs (first row), heart (second row), spleen (third row).

![](https://ars.els-cdn.com/content/image/1-s2.0-S0097849321001187-gr8.jpg)

### Comparison with Real Human Organ

Comparison with real human soft tissues. The living liver in the real video of hepatectomy (left) and the liver rendered using our proposed method (right).

![](https://ars.els-cdn.com/content/image/1-s2.0-S0097849321001187-gr10.jpg)

## references

Guo, Guanhui, Yanni Zou, and Peter X. Liu. "A new rendering algorithm based on multi-space for living soft tissue." *Computers & Graphics* 98 (2021): 242-254.
