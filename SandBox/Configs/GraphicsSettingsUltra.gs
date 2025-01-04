SSAO:
  IsEnabled: true
  AoScale: 1
  Bias: 0.0250000004
  KernelSize: 32
  NoiseSize: 64
  Radius: 0.5
  ResolutionScale: 3
  ResolutionBlurScale: 3
CSM:
  IsEnabled: true
  Quality: 2
  CascadeCount: 5
  SplitFactor: 0.800000012
  MaxDistance: 300
  FogFactor: 0.200000003
  PcfEnabled: true
  PcfRange: 3
  Biases:
    - 0
    - 0
    - 0
    - 0
    - 0
Bloom:
  IsEnabled: true
  MipCount: 10
  BrightnessThreshold: 1
SSR:
  IsEnabled: true
  IsMipMapsEnabled: false
  MaxDistance: 30
  Resolution: 0.400000006
  ResolutionBlurScale: 0
  ResolutionScale: 2
  StepCount: 10
  Thickness: 0.200000003
  BlurRange: 5
  BlurOffset: 5
PostProcess:
  Gamma: 2.20000005
  ToneMapper: 1
  FXAA: true