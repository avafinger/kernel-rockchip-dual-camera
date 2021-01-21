## Kernel 4.19.111 with dual-camera support for NanoPi M4

* Support for OV13850
* Support for OV4689

This kernel is for the NanoPi M4 and to run two cameras.
MIPI CSI-1 and CSI-2 are configured with OV13850 and/or OV4689.

## Sample output / Troubleshooting

1 - Two OV4689 attached

    [    0.815313] ov4689 1-0036: driver version: 00.01.07
    [    0.816267] ov4689 1-0036: 1-0036 supply avdd not found, using dummy regulator
    [    0.817222] ov4689 1-0036: Linked as a consumer to regulator.0
    [    0.817757] ov4689 1-0036: 1-0036 supply dovdd not found, using dummy regulator
    [    0.818444] ov4689 1-0036: 1-0036 supply dvdd not found, using dummy regulator
    [    0.821270] ov4689 1-0036: Detected OV004688 sensor
    [    0.821727] rockchip-mipi-dphy-rx ff770000.syscon:mipi-dphy-rx0: match m00_b_ov4689 1-0036:bus type 4
    [    0.822682] ov4689 2-0036: driver version: 00.01.07
    [    0.823232] ov4689 2-0036: 2-0036 supply avdd not found, using dummy regulator
    [    0.823895] ov4689 2-0036: Linked as a consumer to regulator.0
    [    0.824436] ov4689 2-0036: 2-0036 supply dovdd not found, using dummy regulator
    [    0.825122] ov4689 2-0036: 2-0036 supply dvdd not found, using dummy regulator
    [    0.827929] ov4689 2-0036: Detected OV004688 sensor
    [    0.828396] rockchip-mipi-dphy-rx ff968000.mipi-dphy-tx1rx1: match m02_f_ov4689 2-0036:bus type 4

2 - Streaming

    [  219.400930] rockchip-mipi-dphy-rx ff770000.syscon:mipi-dphy-rx0: stream on:1
    [  219.400936] rockchip-mipi-dphy-rx: data_rate_mbps 1000
    [  219.420343] rockchip-mipi-dphy-rx ff968000.mipi-dphy-tx1rx1: stream on:1
    [  219.420347] rockchip-mipi-dphy-rx: data_rate_mbps 1000
    [  220.155290] rkisp1: Measurement late(18, 17)
    [  220.422891] rkisp1: Measurement late(26, 25)
    [  221.895261] rkisp1: Measurement late(70, 69)
    [  222.006630] rkisp1: Measurement late(71, 70)
    [  222.631493] rkisp1: Measurement late(92, 91)
    [  224.204248] rkisp1: Measurement late(139, 138)
    [  224.237650] rkisp1: Measurement late(140, 139)
    [  225.609616] rkisp1: Measurement late(181, 180)
    [  225.643103] rkisp1: Measurement late(182, 181)
    [  225.810402] rkisp1: Measurement late(187, 186)
    [  226.446261] rkisp1: Measurement late(206, 205)
    [  226.680426] rkisp1: Measurement late(213, 212)
    [  226.682640] rkisp1: MIPI mis error: 0x00800000
    [  227.483522] rkisp1: Measurement late(237, 236)
    [  228.186283] rkisp1: Measurement late(258, 257)
    [  229.022820] rkisp1: Measurement late(283, 282)
    [  229.156684] rkisp1: Measurement late(287, 286)
    [  229.558275] rkisp1: Measurement late(299, 298)
    [  229.625274] rkisp1: Measurement late(301, 300)
    [  229.860512] rkisp1: MIPI mis error: 0x00800000
    [  230.595565] rkisp1: Measurement late(330, 329)
    [  230.729426] rkisp1: Measurement late(334, 333)
    [  231.030568] rkisp1: Measurement late(343, 342)
    [  231.298314] rkisp1: Measurement late(351, 350)
    [  231.309677] rkisp1: Measurement late(349, 348)
    [  231.465598] rkisp1: Measurement late(356, 355)
    [  231.632946] rkisp1: Measurement late(361, 360)
    [  233.172284] rkisp1: Measurement late(407, 406)
    [  233.272586] rkisp1: Measurement late(410, 409)
    [  233.640689] rkisp1: Measurement late(421, 420)
    [  233.741109] rkisp1: Measurement late(424, 423)
    [  233.808004] rkisp1: Measurement late(426, 425)
    [  233.841494] rkisp1: Measurement late(427, 426)
    [  234.176092] rkisp1: Measurement late(437, 436)
    [  234.544194] rkisp1: Measurement late(448, 447)
    [  234.845368] rkisp1: Measurement late(457, 456)
    [  234.912294] rkisp1: Measurement late(459, 458)
    [  237.053911] rkisp1: Measurement late(523, 522)
    [  237.455449] rkisp1: Measurement late(535, 534)
    [  237.756625] rkisp1: Measurement late(544, 543)
    [  237.890490] rkisp1: Measurement late(548, 547)
    [  238.158301] rkisp1: Measurement late(556, 555)
    [  238.593290] rkisp1: Measurement late(569, 568)
    [  238.793966] rkisp1: Measurement late(575, 574)
    [  239.598803] rkisp1: MIPI mis error: 0x00800000
    [  240.901526] rockchip-mipi-dphy-rx ff770000.syscon:mipi-dphy-rx0: stream on:0
    [  242.451939] rockchip-mipi-dphy-rx ff968000.mipi-dphy-tx1rx1: stream on:0
