# 3.5-TFT-LCD-KeDei


![img](http://forum.armbian.com/uploads/monthly_07_2016/post-1122-0-04626400-1467528734.jpg)


![img](http://forum.armbian.com/uploads/monthly_07_2016/post-1122-0-81453900-1467528621.jpg)

I added 3 font:
	16x26
	11x8
	7x10

**ONLY OrangePI!**</br>
required swap the CS TFT and CS touchscreen on-board witch cut wire.</br>
Before you should make sure	that OrangePi enabled SPI,
edited /boot/armbianEnv.txt to add:
<pre><code>
overlay_prefix=sun8i-h3
overlays=spi-spidev
param_spidev_spi_bus=0
param_spidev_max_freq=7000000
</code></pre>

rebooted and found /dev/spidev0.0

after

<pre><code>cd ~ 
git clone https://github.com/maxk9/3.5-TFT-LCD-KeDei.git
cd /3.5-TFT-LCD-KeDei
</code></pre>
and <br/>
<pre><code>sudo make
sudo ./test
</code></pre>

----------






