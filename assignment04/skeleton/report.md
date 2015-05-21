# OS Assignment 04 report
## Please read:
On one hand our code seems to work when commenting out `.getxattr = vfat_fuse_getxattr;` (l. 800) but on the other hand we pass 6/7 instead of 0/7 basic start cluster test when uncommenting, but we get a segfault when trying to `sudo ls -l dest`. So this line is not commented to pass as many public tests as possible.

## 1. Read the rst 512 bytes of the device
Done

## 2. Parse BPB sector
Done

## 3. vfat_next_cluster
Done

## 4.  root directory parsing
Done

## 5.   short  entry  handling
Done

## 6. read from cluster no
Should work

## 7. Integration with the fuse API
Done

## 8. vfat_resolve
Should work

## 9. Long names
Done

## 10.
Done
