SSAO:
  IsEnabled: true
  AoScale: 1
  Bias: 0.0500000007
  KernelSize: 16
  NoiseSize: 64
  Radius: 0.5
  ResolutionScale: 1
  ResolutionBlurScale: 0
CSM:
  IsEnabled: true
  Quality: 1
  CascadeCount: 6
  SplitFactor: 0.899999976
  MaxDistance: 50
  FogFactor: 0.200000003
  Filter: 2
  PcfRange: 2
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
  MaxDistance: 8
  Resolution: 0.5
  ResolutionBlurScale: 1
  ResolutionScale: 1
  StepCount: 10
  Thickness: 0.150000006
  BlurRange: 2
  BlurOffset: 2
  MipMultiplier: 1
PostProcess:
  Gamma: 2.20000005
  ToneMapper: 1
  FXAA: true