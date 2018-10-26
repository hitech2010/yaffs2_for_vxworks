# yaffs2_for_vxworks
yaffs2 for vxworks6.9 on ppc

call yaffs2DiskInit() in usrAppInit.c, vxworks will mount nandflah is a disk named "hd". disk "hd" can be used as ftp server dir. disk "hd" cannot be support as nfs server dir yet.

nandflash driver is not included, it's sample to write one.





