# Paprika Engine
Toy renderer based on DX12 without relying on temporal data (for now).  
  
## Screenshots
### RTX 2070 Laptop @ 1080p
1 sample per pixel without denoising - frametime: 1.99ms
![1_sample.png](Screenshots/1_sample.png)

10 samples per pixel without denoising - frametime: 15.21ms
![10_samples.png](Screenshots/10_samples.png)

5 samples per pixel with 'naive' wip denoising - frametime: 10.60ms
![5_samples_denoised.png](Screenshots/5_samples_denoised.png)