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
SSS:
  IsEnabled: true
  ResolutionScale: 2
  ResolutionBlurScale: 2
  MaxRayDistance: 0.100000001
  MaxDistance: 5
  MaxSteps: 16
  MinThickness: -0.0125000002
  MaxThickness: 0.000150000007
Bloom:
  IsEnabled: true
  MipCount: 10
  BrightnessThreshold: 1
  Intensity: 1
  ResolutionScale: 3
SSR:
  IsEnabled: true
  MaxDistance: 100
  Resolution: 0.5
  ResolutionBlurScale: 2
  ResolutionScale: 1
  StepCount: 20
  Thickness: 5
  BlurRange: 1
  BlurOffset: 1
  MipMultiplier: 0.5
  UseSkyBoxFallback: false
  Blur: 1
PostProcess:
  Gamma: 2.20000005
  ToneMapper: 1
  FXAA: true