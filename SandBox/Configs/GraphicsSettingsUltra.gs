SSAO:
  IsEnabled: true
  AoScale: 1
  Bias: 0.0250000004
  KernelSize: 32
  NoiseSize: 64
  Radius: 0.5
  ResolutionScale: 2
  ResolutionBlurScale: 1
CSM:
  IsEnabled: true
  Quality: 2
  CascadeCount: 3
  SplitFactor: 0.899999976
  MaxDistance: 50
  FogFactor: 0.200000003
  Filter: 2
  PcfRange: 3
  Biases:
    - 0.00400000019
    - 0.0199999996
    - 0.0599999987
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
  ResolutionScale: 2
  StepCount: 10
  Thickness: 0.150000006
  BlurRange: 2
  BlurOffset: 1
PostProcess:
  Gamma: 2.20000005
  ToneMapper: 1
  FXAA: true