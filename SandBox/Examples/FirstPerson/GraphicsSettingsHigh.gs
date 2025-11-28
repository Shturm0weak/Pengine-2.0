SSAO:
  IsEnabled: true
  AoScale: 1
  Bias: 0.0500000007
  KernelSize: 16
  NoiseSize: 64
  Radius: 0.5
  ResolutionScale: 2
  ResolutionBlurScale: 1
CSM:
  IsEnabled: true
  Quality: 1
  CascadeCount: 6
  SplitFactor: 0.899999976
  MaxDistance: 50
  FogFactor: 0.200000003
  Filter: 2
  PcfRange: 1
  Biases:
    - 0.00400000019
    - 0.00899999961
    - 0.00899999961
    - 0.00899999961
    - 0.0199999996
    - 0.100000001
  StabilizeCascades: true
Bloom:
  IsEnabled: true
  MipCount: 10
  BrightnessThreshold: 1
  Intensity: 1
  ResolutionScale: 3
SSR:
  IsEnabled: true
  MaxDistance: 100
  Resolution: 1
  ResolutionBlurScale: 2
  ResolutionScale: 1
  StepCount: 10
  Thickness: 1
  BlurRange: 1
  BlurOffset: 1
  MipMultiplier: 0.5
  UseSkyBoxFallback: true
  Blur: 1
PostProcess:
  Gamma: 2.20000005
  ToneMapper: 1
  FXAA: true