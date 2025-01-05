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
    - 0.0500000007
    - 0.100000001
    - 0.300000012
Bloom:
  IsEnabled: true
  MipCount: 10
  BrightnessThreshold: 1
  Intensity: 1
  ResolutionScale: 1
SSR:
  IsEnabled: true
  MaxDistance: 10
  Resolution: 0.300000012
  ResolutionBlurScale: 0
  ResolutionScale: 0
  StepCount: 10
  Thickness: 1
  BlurRange: 4
  BlurOffset: 4
PostProcess:
  Gamma: 2.20000005
  ToneMapper: 1
  FXAA: true