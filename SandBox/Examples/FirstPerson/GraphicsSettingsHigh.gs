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
  MaxDistance: 50
  FogFactor: 0.200000003
  PcfEnabled: true
  PcfRange: 1
  Biases:
    - 0
    - 0
    - 0
    - 0
Bloom:
  IsEnabled: true
  MipCount: 10
  BrightnessThreshold: 1
  Intensity: 1
  ResolutionScale: 3
SSR:
  IsEnabled: true
  MaxDistance: 15
  Resolution: 0.300000012
  ResolutionBlurScale: 0
  ResolutionScale: 1
  StepCount: 10
  Thickness: 0.300000012
  BlurRange: 5
  BlurOffset: 5
PostProcess:
  Gamma: 2.20000005
  ToneMapper: 1
  FXAA: true