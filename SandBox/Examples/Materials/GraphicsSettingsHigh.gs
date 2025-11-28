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
  CascadeCount: 4
  SplitFactor: 0.899999976
  MaxDistance: 200
  FogFactor: 0.200000003
  Filter: 2
  PcfRange: 2
  Biases:
    - 0.0199999996
    - 0.0399999991
    - 0.150000006
    - 0.400000006
  StabilizeCascades: true
Bloom:
  IsEnabled: true
  MipCount: 10
  BrightnessThreshold: 1
  Intensity: 1
  ResolutionScale: 3
SSR:
  IsEnabled: true
  MaxDistance: 15
  Resolution: 1
  ResolutionBlurScale: 2
  ResolutionScale: 1
  StepCount: 20
  Thickness: 0.150000006
  BlurRange: 1
  BlurOffset: 1
  MipMultiplier: 0.5
  UseSkyBoxFallback: true
  Blur: 1
PostProcess:
  Gamma: 2.20000005
  ToneMapper: 1
  FXAA: true