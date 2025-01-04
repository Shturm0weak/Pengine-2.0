SSAO:
  IsEnabled: true
  AoScale: 1
  Bias: 0.0250000004
  KernelSize: 10
  NoiseSize: 32
  Radius: 0.25
  ResolutionScale: 1
  ResolutionBlurScale: 1
CSM:
  IsEnabled: true
  Quality: 0
  CascadeCount: 3
  SplitFactor: 0.75
  MaxDistance: 100
  FogFactor: 0.200000003
  PcfEnabled: true
  PcfRange: 1
  Biases:
    - 0
    - 0
    - 0
Bloom:
  IsEnabled: true
  MipCount: 8
  BrightnessThreshold: 1
SSR:
  IsEnabled: true
  IsMipMapsEnabled: false
  MaxDistance: 10
  Resolution: 0.300000012
  ResolutionBlurScale: 0
  ResolutionScale: 0
  StepCount: 10
  Thickness: 1
  BlurRange: 2
  BlurOffset: 5
PostProcess:
  Gamma: 2.20000005
  ToneMapper: 1
  FXAA: true