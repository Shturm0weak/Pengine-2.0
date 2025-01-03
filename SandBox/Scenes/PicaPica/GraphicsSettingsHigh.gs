SSAO:
  IsEnabled: true
  AoScale: 1
  Bias: 0.0250000004
  KernelSize: 16
  NoiseSize: 64
  Radius: 0.5
  ResolutionScale: 2
  ResolutionBlurScale: 2
CSM:
  IsEnabled: true
  Quality: 2
  CascadeCount: 4
  SplitFactor: 0.800000012
  MaxDistance: 20
  FogFactor: 0.200000003
  PcfEnabled: true
  PcfRange: 3
  Biases:
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
  MaxDistance: 15
  Resolution: 0.300000012
  ResolutionBlurScale: 0
  ResolutionScale: 1
  StepCount: 10
  Thickness: 5
  BlurRange: 5
  BlurOffset: 5
PostProcess:
  Gamma: 2.20000005
  ToneMapper: 1
  FXAA: true