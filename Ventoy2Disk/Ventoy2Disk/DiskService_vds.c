/******************************************************************************
 * DiskService_vds.c
 *
 * Copyright (c) 2021, longpanda <admin@ventoy.net>
 * Copyright (c) 2011-2020, Pete Batard <pete@akeo.ie>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */
 
#include <Windows.h>
#include <winternl.h>
#include <commctrl.h>
#include <initguid.h>
#include <vds.h>
#include "Ventoy2Disk.h"
#include "DiskService.h"



// Count on Microsoft to add a new API while not bothering updating the existing error facilities,
// so that the new error messages have to be handled manually. Now, since I don't have all day:
// 1. Copy text from https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-vds/5102cc53-3143-4268-ba4c-6ea39e999ab4
// 2. awk '{l[NR%7]=$0} {if (NR%7==0) printf "\tcase %s:\t// %s\n\t\treturn \"%s\";\n", l[1], l[3], l[6]}' vds.txt
// 3. Filter out the crap we don't need.
static const char *GetVdsError(DWORD error_code)
{
	switch (error_code) {
	case 0x80042400:    // VDS_E_NOT_SUPPORTED
    	return "The operation is not supported by the object.";
	case 0x80042401:    // VDS_E_INITIALIZED_FAILED
    	return "VDS or the provider failed to initialize.";
	case 0x80042402:    // VDS_E_INITIALIZE_NOT_CALLED
    	return "VDS did not call the hardware provider's initialization method.";
	case 0x80042403:    // VDS_E_ALREADY_REGISTERED
    	return "The provider is already registered.";
	case 0x80042404:    // VDS_E_ANOTHER_CALL_IN_PROGRESS
    	return "A concurrent second call is made on an object before the first call is completed.";
	case 0x80042405:    // VDS_E_OBJECT_NOT_FOUND
    	return "The specified object was not found.";
	case 0x80042406:    // VDS_E_INVALID_SPACE
    	return "The specified space is neither free nor valid.";
	case 0x80042407:    // VDS_E_PARTITION_LIMIT_REACHED
    	return "No more partitions can be created on the specified disk.";
	case 0x80042408:    // VDS_E_PARTITION_NOT_EMPTY
    	return "The extended partition is not empty.";
	case 0x80042409:    // VDS_E_OPERATION_PENDING
    	return "The operation is still in progress.";
	case 0x8004240A:    // VDS_E_OPERATION_DENIED
    	return "The operation is not permitted on the specified disk, partition, or volume.";
	case 0x8004240B:    // VDS_E_OBJECT_DELETED
    	return "The object no longer exists.";
	case 0x8004240C:    // VDS_E_CANCEL_TOO_LATE
    	return "The operation can no longer be canceled.";
	case 0x8004240D:    // VDS_E_OPERATION_CANCELED
    	return "The operation has already been canceled.";
	case 0x8004240E:    // VDS_E_CANNOT_EXTEND
    	return "The file system does not support extending this volume.";
	case 0x8004240F:    // VDS_E_NOT_ENOUGH_SPACE
    	return "There is not enough space to complete the operation.";
	case 0x80042410:    // VDS_E_NOT_ENOUGH_DRIVE
    	return "There are not enough free disk drives in the subsystem to complete the operation.";
	case 0x80042411:    // VDS_E_BAD_COOKIE
    	return "The cookie was not found.";
	case 0x80042412:    // VDS_E_NO_MEDIA
    	return "There is no removable media in the drive.";
	case 0x80042413:    // VDS_E_DEVICE_IN_USE
    	return "The device is currently in use.";
	case 0x80042414:    // VDS_E_DISK_NOT_EMPTY
    	return "The disk contains partitions or volumes.";
	case 0x80042415:    // VDS_E_INVALID_OPERATION
    	return "The specified operation is not valid.";
	case 0x80042416:    // VDS_E_PATH_NOT_FOUND
    	return "The specified path was not found.";
	case 0x80042417:    // VDS_E_DISK_NOT_INITIALIZED
    	return "The specified disk has not been initialized.";
	case 0x80042418:    // VDS_E_NOT_AN_UNALLOCATED_DISK
    	return "The specified disk is not an unallocated disk.";
	case 0x80042419:    // VDS_E_UNRECOVERABLE_ERROR
    	return "An unrecoverable error occurred. The service MUST shut down.";
	case 0x0004241A:    // VDS_S_DISK_PARTIALLY_CLEANED
    	return "The clean operation was not a full clean or was canceled before it could be completed.";
	case 0x8004241B:    // VDS_E_DMADMIN_SERVICE_CONNECTION_FAILED
    	return "The provider failed to connect to the Logical Disk Manager Administrative service.";
	case 0x8004241C:    // VDS_E_PROVIDER_INITIALIZATION_FAILED
    	return "The provider failed to initialize.";
	case 0x8004241D:    // VDS_E_OBJECT_EXISTS
    	return "The object already exists.";
	case 0x8004241E:    // VDS_E_NO_DISKS_FOUND
    	return "No disks were found on the target machine.";
	case 0x8004241F:    // VDS_E_PROVIDER_CACHE_CORRUPT
    	return "The cache for a provider is corrupt.";
	case 0x80042420:    // VDS_E_DMADMIN_METHOD_CALL_FAILED
    	return "A method call to the Logical Disk Manager Administrative service failed.";
	case 0x00042421:    // VDS_S_PROVIDER_ERROR_LOADING_CACHE
    	return "The provider encountered errors while loading the cache. For more information, see the Windows Event Log.";
	case 0x80042422:    // VDS_E_PROVIDER_VOL_DEVICE_NAME_NOT_FOUND
    	return "The device form of the volume pathname could not be retrieved.";
	case 0x80042423:    // VDS_E_PROVIDER_VOL_OPEN
    	return "Failed to open the volume device";
	case 0x80042424:    // VDS_E_DMADMIN_CORRUPT_NOTIFICATION
    	return "A corrupt notification was sent from the Logical Disk Manager Administrative service.";
	case 0x80042425:    // VDS_E_INCOMPATIBLE_FILE_SYSTEM
    	return "The file system is incompatible with the specified operation.";
	case 0x80042426:    // VDS_E_INCOMPATIBLE_MEDIA
    	return "The media is incompatible with the specified operation.";
	case 0x80042427:    // VDS_E_ACCESS_DENIED
    	return "Access is denied. A VDS operation MUST run under the Backup Operator or Administrators group account.";
	case 0x80042428:    // VDS_E_MEDIA_WRITE_PROTECTED
    	return "The media is write-protected.";
	case 0x80042429:    // VDS_E_BAD_LABEL
    	return "The volume label is not valid.";
	case 0x8004242A:    // VDS_E_CANT_QUICK_FORMAT
    	return "The volume cannot be quick-formatted.";
	case 0x8004242B:    // VDS_E_IO_ERROR
    	return "An I/O error occurred during the operation.";
	case 0x8004242C:    // VDS_E_VOLUME_TOO_SMALL
    	return "The volume size is too small.";
	case 0x8004242D:    // VDS_E_VOLUME_TOO_BIG
    	return "The volume size is too large.";
	case 0x8004242E:    // VDS_E_CLUSTER_SIZE_TOO_SMALL
    	return "The cluster size is too small.";
	case 0x8004242F:    // VDS_E_CLUSTER_SIZE_TOO_BIG
    	return "The cluster size is too large.";
	case 0x80042430:    // VDS_E_CLUSTER_COUNT_BEYOND_32BITS
    	return "The number of clusters is too large to be represented as a 32-bit integer.";
	case 0x80042431:    // VDS_E_OBJECT_STATUS_FAILED
    	return "The component that the object represents has failed and is unable to perform the requested operation.";
	case 0x80042432:    // VDS_E_VOLUME_INCOMPLETE
    	return "The volume is incomplete.";
	case 0x80042433:    // VDS_E_EXTENT_SIZE_LESS_THAN_MIN
    	return "The specified extent size is too small.";
	case 0x00042434:    // VDS_S_UPDATE_BOOTFILE_FAILED
    	return "The operation was successful, but VDS failed to update the boot options in the Boot Configuration Data (BCD) store or boot.ini file.";
	case 0x00042436:    // VDS_S_BOOT_PARTITION_NUMBER_CHANGE
    	return "The boot partition's partition number will change as a result of the operation.";
	case 0x80042436:    // VDS_E_BOOT_PARTITION_NUMBER_CHANGE
    	return "The boot partition's partition number will change as a result of the migration operation.";
	case 0x80042437:    // VDS_E_NO_FREE_SPACE
    	return "The specified disk does not have enough free space to complete the operation.";
	case 0x80042438:    // VDS_E_ACTIVE_PARTITION
    	return "An active partition was detected on the selected disk, and it is not the active partition that was used to boot the active operating system.";
	case 0x80042439:    // VDS_E_PARTITION_OF_UNKNOWN_TYPE
    	return "The partition information cannot be read.";
	case 0x8004243A:    // VDS_E_LEGACY_VOLUME_FORMAT
    	return "A partition with an unknown type was detected on the specified disk.";
	case 0x8004243B:    // VDS_E_NON_CONTIGUOUS_DATA_PARTITIONS
    	return "The selected GPT disk contains two basic data partitions that are separated by an OEM partition.";
	case 0x8004243C:    // VDS_E_MIGRATE_OPEN_VOLUME
    	return "A volume on the specified disk could not be opened.";
	case 0x8004243D:    // VDS_E_VOLUME_NOT_ONLINE
    	return "The volume is not online.";
	case 0x8004243E:    // VDS_E_VOLUME_NOT_HEALTHY
    	return "The volume is failing or has failed.";
	case 0x8004243F:    // VDS_E_VOLUME_SPANS_DISKS
    	return "The volume spans multiple disks.";
	case 0x80042440:    // VDS_E_REQUIRES_CONTIGUOUS_DISK_SPACE
    	return "The volume consists of multiple disk extents. The operation failed because it requires the volume to consist of a single disk extent.";
	case 0x80042441:    // VDS_E_BAD_PROVIDER_DATA
    	return "A provider returned bad data.";
	case 0x80042442:    // VDS_E_PROVIDER_FAILURE
    	return "A provider failed to complete an operation.";
	case 0x00042443:    // VDS_S_VOLUME_COMPRESS_FAILED
    	return "The file system was formatted successfully but could not be compressed.";
	case 0x80042444:    // VDS_E_PACK_OFFLINE
    	return "The pack is offline.";
	case 0x80042445:    // VDS_E_VOLUME_NOT_A_MIRROR
    	return "The volume is not a mirror.";
	case 0x80042446:    // VDS_E_NO_EXTENTS_FOR_VOLUME
    	return "No extents were found for the volume.";
	case 0x80042447:    // VDS_E_DISK_NOT_LOADED_TO_CACHE
    	return "The migrated disk failed to load to the cache.";
	case 0x80042448:    // VDS_E_INTERNAL_ERROR
    	return "VDS encountered an internal error. For more information, see the Windows Event Log.";
	case 0x8004244A:    // VDS_E_PROVIDER_TYPE_NOT_SUPPORTED
    	return "The method call is not supported for the specified provider type.";
	case 0x8004244B:    // VDS_E_DISK_NOT_ONLINE
    	return "One or more of the specified disks are not online.";
	case 0x8004244C:    // VDS_E_DISK_IN_USE_BY_VOLUME
    	return "One or more extents of the disk are already being used by the volume.";
	case 0x0004244D:    // VDS_S_IN_PROGRESS
    	return "The asynchronous operation is in progress.";
	case 0x8004244E:    // VDS_E_ASYNC_OBJECT_FAILURE
    	return "Failure initializing the asynchronous object.";
	case 0x8004244F:    // VDS_E_VOLUME_NOT_MOUNTED
    	return "The volume is not mounted.";
	case 0x80042450:    // VDS_E_PACK_NOT_FOUND
    	return "The pack was not found.";
	case 0x80042451:    // VDS_E_IMPORT_SET_INCOMPLETE
    	return "An attempt was made to import a subset of the disks in the foreign pack.";
	case 0x80042452:    // VDS_E_DISK_NOT_IMPORTED
    	return "A disk in the import's source pack was not imported.";
	case 0x80042453:    // VDS_E_OBJECT_OUT_OF_SYNC
    	return "The reference to the object might be stale.";
	case 0x80042454:    // VDS_E_MISSING_DISK
    	return "The specified disk could not be found.";
	case 0x80042455:    // VDS_E_DISK_PNP_REG_CORRUPT
    	return "The provider's list of PnP registered disks has become corrupted.";
	case 0x80042456:    // VDS_E_LBN_REMAP_ENABLED_FLAG
    	return "The provider does not support the VDS_VF_LBN REMAP_ENABLED volume flag.";
	case 0x80042457:    // VDS_E_NO_DRIVELETTER_FLAG
    	return "The provider does not support the VDS_VF_NO DRIVELETTER volume flag.";
	case 0x80042458:    // VDS_E_REVERT_ON_CLOSE
    	return "The bRevertOnClose parameter can only be set to TRUE if the VDS_VF_HIDDEN, VDS_VF_READONLY, VDS_VF_NO_DEFAULT_DRIVE_LETTER, or VDS_VF_SHADOW_COPY volume flag is set in the ulFlags parameter. For more information, see IVdsVolume::SetFlags.";
	case 0x80042459:    // VDS_E_REVERT_ON_CLOSE_SET
    	return "Some volume flags are already set. The software clears these flags first, then calls IVdsVolume::SetFlags again, specifying TRUE for the bRevertOnClose parameter.";
	case 0x8004245A:    // VDS_E_IA64_BOOT_MIRRORED_TO_MBR
    	return "Not used. The boot volume has been mirrored on a GPT disk to an MBR disk. The machine will not be bootable from the secondary plex.";
	case 0x0004245A:    // VDS_S_IA64_BOOT_MIRRORED_TO_MBR
    	return "The boot volume has been mirrored on a GPT disk to an MBR disk. The machine will not be bootable from the secondary plex.";
	case 0x0004245B:    // VDS_S_UNABLE_TO_GET_GPT_ATTRIBUTES
    	return "Unable to retrieve the GPT attributes for this volume, (hidden, read only and no drive letter).";
	case 0x8004245C:    // VDS_E_VOLUME_TEMPORARILY_DISMOUNTED
    	return "The volume is already dismounted temporarily.";
	case 0x8004245D:    // VDS_E_VOLUME_PERMANENTLY_DISMOUNTED
    	return "The volume is already permanently dismounted. It cannot be dismounted temporarily until it becomes mountable.";
	case 0x8004245E:    // VDS_E_VOLUME_HAS_PATH
    	return "The volume cannot be dismounted permanently because it still has an access path.";
	case 0x8004245F:    // VDS_E_TIMEOUT
    	return "The operation timed out.";
	case 0x80042460:    // VDS_E_REPAIR_VOLUMESTATE
    	return "The volume plex cannot be repaired. The volume and plex MUST be online, and MUST not be healthy or rebuilding.";
	case 0x80042461:    // VDS_E_LDM_TIMEOUT
    	return "The operation timed out in the Logical Disk Manager Administrative service. Retry the operation.";
	case 0x80042462:    // VDS_E_REVERT_ON_CLOSE_MISMATCH
    	return "The flags to be cleared do not match the flags that were set previously when the IVdsVolume::SetFlags method was called with the bRevertOnClose parameter set to TRUE.";
	case 0x80042463:    // VDS_E_RETRY
    	return "The operation failed. Retry the operation.";
	case 0x80042464:    // VDS_E_ONLINE_PACK_EXISTS
    	return "The operation failed, because an online pack object already exists.";
	case 0x00042465:    // VDS_S_EXTEND_FILE_SYSTEM_FAILED
    	return "The volume was extended successfully but the file system failed to extend.";
	case 0x80042466:    // VDS_E_EXTEND_FILE_SYSTEM_FAILED
    	return "The file system failed to extend.";
	case 0x00042467:    // VDS_S_MBR_BOOT_MIRRORED_TO_GPT
    	return "The boot volume has been mirrored on an MBR disk to a GPT disk. The machine will not be bootable from the secondary plex.";
	case 0x80042468:    // VDS_E_MAX_USABLE_MBR
    	return "Only the first 2TB are usable on large MBR disks. Cannot create partitions beyond the 2TB mark, nor convert the disk to dynamic.";
	case 0x00042469:    // VDS_S_GPT_BOOT_MIRRORED_TO_MBR
    	return "The boot volume on a GPT disk has been mirrored to an MBR disk. The new plex cannot be used to boot the computer.";
	case 0x80042500:    // VDS_E_NO_SOFTWARE_PROVIDERS_LOADED
    	return "There are no software providers loaded.";
	case 0x80042501:    // VDS_E_DISK_NOT_MISSING
    	return "The disk is not missing.";
	case 0x80042502:    // VDS_E_NO_VOLUME_LAYOUT
    	return "The volume's layout could not be retrieved.";
	case 0x80042503:    // VDS_E_CORRUPT_VOLUME_INFO
    	return "The volume's driver information is corrupted.";
	case 0x80042504:    // VDS_E_INVALID_ENUMERATOR
    	return "The enumerator is corrupted";
	case 0x80042505:    // VDS_E_DRIVER_INTERNAL_ERROR
    	return "An internal error occurred in the volume management driver.";
	case 0x80042507:    // VDS_E_VOLUME_INVALID_NAME
    	return "The volume name is not valid.";
	case 0x00042508:    // VDS_S_DISK_IS_MISSING
    	return "The disk is missing and not all information could be returned.";
	case 0x80042509:    // VDS_E_CORRUPT_PARTITION_INFO
    	return "The disk's partition information is corrupted.";
	case 0x0004250A:    // VDS_S_NONCONFORMANT_PARTITION_INFO
    	return "The disk's partition information does not conform to what is expected on a dynamic disk. The disk's partition information is corrupted.";
	case 0x8004250B:    // VDS_E_CORRUPT_EXTENT_INFO
    	return "The disk's extent information is corrupted.";
	case 0x8004250C:    // VDS_E_DUP_EMPTY_PACK_GUID
    	return "An empty pack already exists. Release the existing empty pack before creating another empty pack.";
	case 0x8004250D:    // VDS_E_DRIVER_NO_PACK_NAME
    	return "The volume management driver did not return a pack name. Internal driver error.";
	case 0x0004250E:    // VDS_S_SYSTEM_PARTITION
    	return "Warning: There was a failure while checking for the system partition.";
	case 0x8004250F:    // VDS_E_BAD_PNP_MESSAGE
    	return "The PNP service sent a corrupted notification to the provider.";
	case 0x80042510:    // VDS_E_NO_PNP_DISK_ARRIVE
    	return "No disk arrival notification was received.";
	case 0x80042511:    // VDS_E_NO_PNP_VOLUME_ARRIVE
    	return "No volume arrival notification was received.";
	case 0x80042512:    // VDS_E_NO_PNP_DISK_REMOVE
    	return "No disk removal notification was received.";
	case 0x80042513:    // VDS_E_NO_PNP_VOLUME_REMOVE
    	return "No volume removal notification was received.";
	case 0x80042514:    // VDS_E_PROVIDER_EXITING
    	return "The provider is exiting.";
	case 0x80042515:    // VDS_E_EXTENT_EXCEEDS_DISK_FREE_SPACE
    	return "The specified disk extent size is larger than the amount of free disk space.";
	case 0x80042516:    // VDS_E_MEMBER_SIZE_INVALID
    	return "The specified plex member size is not valid.";
	case 0x00042517:    // VDS_S_NO_NOTIFICATION
    	return "No volume arrival notification was received. The software might need to call IVdsService::Refresh.";
	case 0x00042518:    // VDS_S_DEFAULT_PLEX_MEMBER_IDS
    	return "Defaults have been used for the member ids or plex ids.";
	case 0x80042519:    // VDS_E_INVALID_DISK
    	return "The specified disk is not valid.";
	case 0x8004251A:    // VDS_E_INVALID_PACK
    	return "The specified disk pack is not valid.";
	case 0x8004251B:    // VDS_E_VOLUME_ON_DISK
    	return "This operation is not allowed on disks with volumes.";
	case 0x8004251C:    // VDS_E_DRIVER_INVALID_PARAM
    	return "The driver returned an invalid parameter error.";
	case 0x8004251D:    // VDS_E_TARGET_PACK_NOT_EMPTY
    	return "The target pack is not empty.";
	case 0x8004251E:    // VDS_E_CANNOT_SHRINK
    	return "The file system does not support shrinking this volume.";
	case 0x8004251F:    // VDS_E_MULTIPLE_PACKS
    	return "Specified disks are not all from the same pack.";
	case 0x80042520:    // VDS_E_PACK_ONLINE
    	return "This operation is not allowed on online packs. The pack MUST be offline.";
	case 0x80042521:    // VDS_E_INVALID_PLEX_COUNT
    	return "The plex count for the volume MUST be greater than zero.";
	case 0x80042522:    // VDS_E_INVALID_MEMBER_COUNT
    	return "The member count for the volume MUST be greater than zero.";
	case 0x80042523:    // VDS_E_INVALID_PLEX_ORDER
    	return "The plex indexes MUST start at zero and increase monotonically.";
	case 0x80042524:    // VDS_E_INVALID_MEMBER_ORDER
    	return "The member indexes MUST start at zero and increase monotonically.";
	case 0x80042525:    // VDS_E_INVALID_STRIPE_SIZE
    	return "The stripe size in bytes MUST be a power of 2 for striped and RAID-5 volume types and zero for all other volume types.";
	case 0x80042526:    // VDS_E_INVALID_DISK_COUNT
    	return "The number of disks specified is not valid for this operation.";
	case 0x80042527:    // VDS_E_INVALID_EXTENT_COUNT
    	return "An invalid number of extents was specified for at least one disk.";
	case 0x80042528:    // VDS_E_SOURCE_IS_TARGET_PACK
    	return "The source and target packs MUST be distinct.";
	case 0x80042529:    // VDS_E_VOLUME_DISK_COUNT_MAX_EXCEEDED
    	return "The specified number of disks is too large. VDS imposes a 32-disk limit on spanned, striped, and striped with parity (RAID-5) volumes.";
	case 0x8004252A:    // VDS_E_CORRUPT_NOTIFICATION_INFO
    	return "The driver's notification information is corrupt.";
	case 0x8004252C:    // VDS_E_INVALID_PLEX_GUID
    	return "GUID_NULL is not a valid plex GUID.";
	case 0x8004252D:    // VDS_E_DISK_NOT_FOUND_IN_PACK
    	return "The specified disks do not belong to the same pack.";
	case 0x8004252E:    // VDS_E_DUPLICATE_DISK
    	return "The same disk was specified more than once.";
	case 0x8004252F:    // VDS_E_LAST_VALID_DISK
    	return "The operation cannot be completed because there is only one valid disk in the pack.";
	case 0x80042530:    // VDS_E_INVALID_SECTOR_SIZE
    	return "All disks holding extents for a given volume MUST have the same valid sector size.";
	case 0x80042531:    // VDS_E_ONE_EXTENT_PER_DISK
    	return "A single disk cannot contribute to multiple members or multiple plexes of the same volume.";
	case 0x80042532:    // VDS_E_INVALID_BLOCK_SIZE
    	return "Neither the volume stripe size nor the disk sector size was found to be non-zero.";
	case 0x80042533:    // VDS_E_PLEX_SIZE_INVALID
    	return "The size of the volume plex is invalid.";
	case 0x80042534:    // VDS_E_NO_EXTENTS_FOR_PLEX
    	return "No extents were found for the plex.";
	case 0x80042535:    // VDS_E_INVALID_PLEX_TYPE
    	return "The plex type is invalid.";
	case 0x80042536:    // VDS_E_INVALID_PLEX_BLOCK_SIZE
    	return "The plex block size MUST be nonzero.";
	case 0x80042537:    // VDS_E_NO_HEALTHY_DISKS
    	return "All of the disks involved in the operation are either missing or failed.";
	case 0x80042538:    // VDS_E_CONFIG_LIMIT
    	return "The Logical Disk Management database is full and no more volumes or disks can be configured.";
	case 0x80042539:    // VDS_E_DISK_CONFIGURATION_CORRUPTED
    	return "The disk configuration data is corrupted.";
	case 0x8004253A:    // VDS_E_DISK_CONFIGURATION_NOT_IN_SYNC
    	return "The disk configuration is not in sync with the in-memory configuration.";
	case 0x8004253B:    // VDS_E_DISK_CONFIGURATION_UPDATE_FAILED
    	return "One or more disks failed to be updated with the new configuration.";
	case 0x8004253C:    // VDS_E_DISK_DYNAMIC
    	return "The disk is already dynamic.";
	case 0x8004253D:    // VDS_E_DRIVER_OBJECT_NOT_FOUND
    	return "The object was not found in the driver cache.";
	case 0x8004253E:    // VDS_E_PARTITION_NOT_CYLINDER_ALIGNED
    	return "The disk layout contains partitions which are not cylinder aligned.";
	case 0x8004253F:    // VDS_E_DISK_LAYOUT_PARTITIONS_TOO_SMALL
    	return "The disk layout contains partitions which are less than the minimum required size.";
	case 0x80042540:    // VDS_E_DISK_IO_FAILING
    	return "The IO to the disk is failing.";
	case 0x80042541:    // VDS_E_DYNAMIC_DISKS_NOT_SUPPORTED
    	return "Dynamic disks are not supported by this operating system or server configuration. Dynamic disks are not supported on clusters.";
	case 0x80042542:    // VDS_E_FAULT_TOLERANT_DISKS_NOT_SUPPORTED
    	return "The fault tolerant disks are not supported by this operating system.";
	case 0x80042543:    // VDS_E_GPT_ATTRIBUTES_INVALID
    	return "Invalid GPT attributes were specified.";
	case 0x80042544:    // VDS_E_MEMBER_IS_HEALTHY
    	return "The member is not stale or detached.";
	case 0x80042545:    // VDS_E_MEMBER_REGENERATING
    	return "The member is regenerating.";
	case 0x80042546:    // VDS_E_PACK_NAME_INVALID
    	return "The pack name is invalid.";
	case 0x80042547:    // VDS_E_PLEX_IS_HEALTHY
    	return "The plex is not stale or detached.";
	case 0x80042548:    // VDS_E_PLEX_LAST_ACTIVE
    	return "The last healthy plex cannot be removed.";
	case 0x80042549:    // VDS_E_PLEX_MISSING
    	return "The plex is missing.";
	case 0x8004254A:    // VDS_E_MEMBER_MISSING
    	return "The member is missing.";
	case 0x8004254B:    // VDS_E_PLEX_REGENERATING
    	return "The plex is regenerating.";
	case 0x8004254D:    // VDS_E_UNEXPECTED_DISK_LAYOUT_CHANGE
    	return "An unexpected layout change occurred external to the volume manager.";
	case 0x8004254E:    // VDS_E_INVALID_VOLUME_LENGTH
    	return "The volume length is invalid.";
	case 0x8004254F:    // VDS_E_VOLUME_LENGTH_NOT_SECTOR_SIZE_MULTIPLE
    	return "The volume length is not a multiple of the sector size.";
	case 0x80042550:    // VDS_E_VOLUME_NOT_RETAINED
    	return "The volume does not have a retained partition association.";
	case 0x80042551:    // VDS_E_VOLUME_RETAINED
    	return "The volume already has a retained partition association.";
	case 0x80042553:    // VDS_E_ALIGN_BEYOND_FIRST_CYLINDER
    	return "The specified alignment is beyond the first cylinder.";
	case 0x80042554:    // VDS_E_ALIGN_NOT_SECTOR_SIZE_MULTIPLE
    	return "The specified alignment is not a multiple of the sector size.";
	case 0x80042555:    // VDS_E_ALIGN_NOT_ZERO
    	return "The specified partition type cannot be created with a non-zero alignment.";
	case 0x80042556:    // VDS_E_CACHE_CORRUPT
    	return "The service's cache has become corrupt.";
	case 0x80042557:    // VDS_E_CANNOT_CLEAR_VOLUME_FLAG
    	return "The specified volume flag cannot be cleared.";
	case 0x80042558:    // VDS_E_DISK_BEING_CLEANED
    	return "The operation is not allowed on a disk that is in the process of being cleaned.";
	case 0x80042559:    // VDS_E_DISK_NOT_CONVERTIBLE
    	return "The specified disk is not convertible. CDROMs and DVDs are examples of disk that are not convertible.";
	case 0x8004255A:    // VDS_E_DISK_REMOVEABLE
    	return "The operation is not supported on removable media.";
	case 0x8004255B:    // VDS_E_DISK_REMOVEABLE_NOT_EMPTY
    	return "The operation is not supported on a non-empty removable disk.";
	case 0x8004255C:    // VDS_E_DRIVE_LETTER_NOT_FREE
    	return "The specified drive letter is not free to be assigned.";
	case 0x8004255D:    // VDS_E_EXTEND_MULTIPLE_DISKS_NOT_SUPPORTED
    	return "Extending the volume onto multiple disks is not supported by this provider.";
	case 0x8004255E:    // VDS_E_INVALID_DRIVE_LETTER
    	return "The specified drive letter is not valid.";
	case 0x8004255F:    // VDS_E_INVALID_DRIVE_LETTER_COUNT
    	return "The specified number of drive letters to retrieve is not valid.";
	case 0x80042560:    // VDS_E_INVALID_FS_FLAG
    	return "The specified file system flag is not valid.";
	case 0x80042561:    // VDS_E_INVALID_FS_TYPE
    	return "The specified file system is not valid.";
	case 0x80042562:    // VDS_E_INVALID_OBJECT_TYPE
    	return "The specified object type is not valid.";
	case 0x80042563:    // VDS_E_INVALID_PARTITION_LAYOUT
    	return "The specified partition layout is invalid.";
	case 0x80042564:    // VDS_E_INVALID_PARTITION_STYLE
    	return "VDS only supports MBR or GPT partition style disks.";
	case 0x80042565:    // VDS_E_INVALID_PARTITION_TYPE
    	return "The specified partition type is not valid for this operation.";
	case 0x80042566:    // VDS_E_INVALID_PROVIDER_CLSID
    	return "The specified provider clsid cannot be a NULL GUID.";
	case 0x80042567:    // VDS_E_INVALID_PROVIDER_ID
    	return "The specified provider id cannot be a NULL GUID.";
	case 0x80042568:    // VDS_E_INVALID_PROVIDER_NAME
    	return "The specified provider name is invalid.";
	case 0x80042569:    // VDS_E_INVALID_PROVIDER_TYPE
    	return "The specified provider type is invalid.";
	case 0x8004256A:    // VDS_E_INVALID_PROVIDER_VERSION_GUID
    	return "The specified provider version GUID cannot be a NULL GUID.";
	case 0x8004256B:    // VDS_E_INVALID_PROVIDER_VERSION_STRING
    	return "The specified provider version string is invalid.";
	case 0x8004256C:    // VDS_E_INVALID_QUERY_PROVIDER_FLAG
    	return "The specified query provider flag is invalid.";
	case 0x8004256D:    // VDS_E_INVALID_SERVICE_FLAG
    	return "The specified service flag is invalid.";
	case 0x8004256E:    // VDS_E_INVALID_VOLUME_FLAG
    	return "The specified volume flag is invalid.";
	case 0x8004256F:    // VDS_E_PARTITION_NOT_OEM
    	return "The operation is only supported on an OEM, ESP, or unknown partition.";
	case 0x80042570:    // VDS_E_PARTITION_PROTECTED
    	return "Cannot delete a protected partition without the force protected parameter set, (see bForceProtected parameter in IVdsAdvancedDisk::DeletePartition).";
	case 0x80042571:    // VDS_E_PARTITION_STYLE_MISMATCH
    	return "The specified partition style is not the same as the disk's partition style.";
	case 0x80042572:    // VDS_E_PROVIDER_INTERNAL_ERROR
    	return "An internal error has occurred in the provider.";
	case 0x80042573:    // VDS_E_SHRINK_SIZE_LESS_THAN_MIN
    	return "The specified shrink size is less than the minimum shrink size allowed.";
	case 0x80042574:    // VDS_E_SHRINK_SIZE_TOO_BIG
    	return "The specified shrink size is too large and will cause the volume to be smaller than the minimum volume size.";
	case 0x80042575:    // VDS_E_UNRECOVERABLE_PROVIDER_ERROR
    	return "An unrecoverable error occurred in a provider.  The service MUST be shut down to regain full functionality.";
	case 0x80042576:    // VDS_E_VOLUME_HIDDEN
    	return "Cannot assign a mount point to a hidden volume.";
	case 0x00042577:    // VDS_S_DISMOUNT_FAILED
    	return "Failed to dismount the volume after setting the volume flags.";
	case 0x00042578:    // VDS_S_REMOUNT_FAILED
    	return "Failed to remount the volume after setting the volume flags.";
	case 0x80042579:    // VDS_E_FLAG_ALREADY_SET
    	return "Cannot set the specified flag as revert-on-close because it is already set. For more information, see the bRevertOnClose parameter of IVdsVolume::SetFlags.";
	case 0x0004257A:    // VDS_S_RESYNC_NOTIFICATION_TASK_FAILED
    	return "Failure. If the volume is a mirror volume or a raid5 volume, no resynchronization notifications will be sent.";
	case 0x8004257B:    // VDS_E_DISTINCT_VOLUME
    	return "The input volume id cannot be the id of the volume that is the target of the operation.";
	case 0x8004257C:    // VDS_E_VOLUME_NOT_FOUND_IN_PACK
    	return "The specified volumes do not belong to the same pack.";
	case 0x8004257D:    // VDS_E_PARTITION_NON_DATA
    	return "The specified partition is a not a primary or logical volume.";
	case 0x8004257E:    // VDS_E_CRITICAL_PLEX
    	return "The specified plex is the current system or boot plex.";
	case 0x8004257F:    // VDS_E_VOLUME_SYNCHRONIZING
    	return "The operation cannot be completed because the volume is synchronizing.";
	case 0x80042580:    // VDS_E_VOLUME_REGENERATING
    	return "The operation cannot be completed because the volume is regenerating.";
	case 0x00042581:    // VDS_S_VSS_FLUSH_AND_HOLD_WRITES
    	return "Failed to flush and hold Volume Snapshot Service writes.";
	case 0x00042582:    // VDS_S_VSS_RELEASE_WRITES
    	return "Failed to release Volume Snapshot Service writes.";
	case 0x00042583:    // VDS_S_FS_LOCK
    	return "Failed to obtain a file system lock.";
	case 0x80042584:    // VDS_E_READONLY
    	return "The volume is read only.";
	case 0x80042585:    // VDS_E_INVALID_VOLUME_TYPE
    	return "The volume type is invalid for this operation.";
	case 0x80042586:    // VDS_E_BAD_BOOT_DISK
    	return "The boot disk experienced failures when the driver attempted to online the pack.";
	case 0x80042587:    // VDS_E_LOG_UPDATE
    	return "The driver failed to update the log on at least one disk.";
	case 0x80042588:    // VDS_E_VOLUME_MIRRORED
    	return "This operation is not supported on a mirrored volume.";
	case 0x80042589:    // VDS_E_VOLUME_SIMPLE_SPANNED
    	return "The operation is only supported on simple or spanned volumes.";
	case 0x8004258A:    // VDS_E_NO_VALID_LOG_COPIES
    	return "This pack has no valid log copies.";
	case 0x0004258B:    // VDS_S_PLEX_NOT_LOADED_TO_CACHE
    	return "This plex is present in the driver, but has not yet been loaded to the provider cache. A volume modified notification will be sent by the service once the plex has been loaded to the provider cache.";
	case 0x8004258B:    // VDS_E_PLEX_NOT_LOADED_TO_CACHE
    	return "This plex is present in the driver, but has not yet been loaded to the provider cache. A volume modified notification will be sent by the service once the plex has been loaded to the provider cache.";
	case 0x8004258C:    // VDS_E_PARTITION_MSR
    	return "The operation is not supported on MSR partitions.";
	case 0x8004258D:    // VDS_E_PARTITION_LDM
    	return "The operation is not supported on LDM partitions.";
	case 0x0004258E:    // VDS_S_WINPE_BOOTENTRY
    	return "The boot entries cannot be updated automatically on WinPE. It might be necessary to manually update the boot entry for any installed operating systems.";
	case 0x8004258F:    // VDS_E_ALIGN_NOT_A_POWER_OF_TWO
    	return "The alignment is not a power of two.";
	case 0x80042590:    // VDS_E_ALIGN_IS_ZERO
    	return "The alignment is zero.";
	case 0x80042591:    // VDS_E_SHRINK_IN_PROGRESS
    	return "A defragmentation or volume shrink operation is already in progress. Only one of these operations can run at a time.";
	case 0x80042592:    // VDS_E_CANT_INVALIDATE_FVE
    	return "BitLocker encryption could not be disabled for the volume.";
	case 0x80042593:    // VDS_E_FS_NOT_DETERMINED
    	return "The default file system could not be determined.";
	case 0x80042595:    // VDS_E_DISK_NOT_OFFLINE
    	return "This disk is already online.";
	case 0x80042596:    // VDS_E_FAILED_TO_ONLINE_DISK
    	return "The online operation failed.";
	case 0x80042597:    // VDS_E_FAILED_TO_OFFLINE_DISK
    	return "The offline operation failed.";
	case 0x80042598:    // VDS_E_BAD_REVISION_NUMBER
    	return "The operation could not be completed because the specified revision number is not supported.";
	case 0x80042599:    // VDS_E_SHRINK_USER_CANCELLED
    	return "The shrink operation was canceled by the user.";
	case 0x8004259A:    // VDS_E_SHRINK_DIRTY_VOLUME
    	return "The volume selected for shrink might be corrupted. Use a file system repair utility to fix the corruption problem and then try to shrink the volume again.";
	case 0x00042700:    // VDS_S_NAME_TRUNCATED
    	return "The name was set successfully but had to be truncated.";
	case 0x80042701:    // VDS_E_NAME_NOT_UNIQUE
    	return "The specified name is not unique.";
	case 0x00042702:    // VDS_S_STATUSES_INCOMPLETELY_SET
    	return "At least one path's status was not successfully set due to a nonfatal error (for example, the status conflicts with the current load balance policy).";
	case 0x80042703:    // VDS_E_ADDRESSES_INCOMPLETELY_SET
    	return "At least one portal's tunnel address, which is the address of a portal that is running IPsec in tunnel mode, is not set successfully.";
	case 0x80042705:    // VDS_E_SECURITY_INCOMPLETELY_SET
    	return "At least one portal's security settings are not set successfully.";
	case 0x80042706:    // VDS_E_TARGET_SPECIFIC_NOT_SUPPORTED
    	return "The initiator service does not support setting target-specific shared secrets.";
	case 0x80042707:    // VDS_E_INITIATOR_SPECIFIC_NOT_SUPPORTED
    	return "The target does not support setting initiator-specific shared secrets.";
	case 0x80042708:    // VDS_E_ISCSI_LOGIN_FAILED
    	return "Another operation is in progress. This operation cannot proceed until the previous operations are complete.";
	case 0x80042709:    // VDS_E_ISCSI_LOGOUT_FAILED
    	return "The attempt to log out from the specified iSCSI session failed.";
	case 0x8004270A:    // VDS_E_ISCSI_SESSION_NOT_FOUND
    	return "VDS could not find a session matching the specified iSCSI target.";
	case 0x8004270B:    // VDS_E_ASSOCIATED_LUNS_EXIST
    	return "LUNs are associated with this target. All LUNs MUST be disassociated from this target before the target can be deleted.";
	case 0x8004270C:    // VDS_E_ASSOCIATED_PORTALS_EXIST
    	return "Portals are associated with this portal group. All portals MUST be disassociated from this portal group before the portal group can be deleted.";
	case 0x8004270D:    // VDS_E_NO_DISCOVERY_DOMAIN
    	return "The initiator does not exist in an iSNS discovery domain.";
	case 0x8004270E:    // VDS_E_MULTIPLE_DISCOVERY_DOMAINS
    	return "The initiator exists in more than one iSNS discovery domain.";
	case 0x8004270F:    // VDS_E_NO_DISK_PATHNAME
    	return "The disk's path could not be retrieved. Some operations on the disk might fail.";
	case 0x80042710:    // VDS_E_ISCSI_LOGOUT_INCOMPLETE
    	return "At least one iSCSI session logout operation did not complete successfully.";
	case 0x80042711:    // VDS_E_NO_VOLUME_PATHNAME
    	return "The path could not be retrieved for one or more volumes.";
	case 0x80042712:    // VDS_E_PROVIDER_CACHE_OUTOFSYNC
    	return "The provider's cache is not in sync with the driver cache.";
	case 0x80042713:    // VDS_E_NO_IMPORT_TARGET
    	return "No import target was set for the subsystem.";
	case 0x00042714:    // VDS_S_ALREADY_EXISTS
    	return "The object already exists.";
	case 0x00042715:    // VDS_S_PROPERTIES_INCOMPLETE
    	return "Some, but not all, of the properties were successfully retrieved. Note that there are many possible reasons for failing to retrieve all properties, including device removal.";
	case 0x00042800:    // VDS_S_ISCSI_SESSION_NOT_FOUND_PERSISTENT_LOGIN_REMOVED
    	return "VDS could not find any sessions matching the specified iSCSI target, but one or more persistent logins were found and removed.";
	case 0x00042801:    // VDS_S_ISCSI_PERSISTENT_LOGIN_MAY_NOT_BE_REMOVED
    	return "If a persistent login was set up for the target, it possibly was not removed. Check the iSCSI Initiator Control Panel to remove it if necessary.";
	case 0x00042802:    // VDS_S_ISCSI_LOGIN_ALREAD_EXISTS
    	return "The attempt to log in to the iSCSI target failed because the session already exists.";
	case 0x80042803:    // VDS_E_UNABLE_TO_FIND_BOOT_DISK
    	return "Volume disk extent information could not be retrieved for the boot volume.";
	case 0x80042804:    // VDS_E_INCORRECT_BOOT_VOLUME_EXTENT_INFO
    	return "More than two disk extents were reported for the boot volume. This is a system error.";
	case 0x80042805:    // VDS_E_GET_SAN_POLICY
    	return "A driver error was reported when getting the SAN policy.";
	case 0x80042806:    // VDS_E_SET_SAN_POLICY
    	return "A driver error was reported when setting the SAN policy.";
	case 0x80042807:    // VDS_E_BOOT_DISK
    	return "Disk attributes cannot be changed on the boot disk.";
	case 0x00042808:    // VDS_S_DISK_MOUNT_FAILED
    	return "One or more of the volumes on the disk could not be mounted, possibly because it was already mounted.";
	case 0x00042809:    // VDS_S_DISK_DISMOUNT_FAILED
    	return "One or more of the volumes on the disk could not be dismounted, possibly because it was already dismounted.";
	case 0x8004280A:    // VDS_E_DISK_IS_OFFLINE
    	return "The operation cannot be performed on a disk that is offline.";
	case 0x8004280B:    // VDS_E_DISK_IS_READ_ONLY
    	return "The operation cannot be performed on a disk that is read-only.";
	case 0x8004280C:    // VDS_E_PAGEFILE_DISK
    	return "The operation cannot be performed on a disk that contains a pagefile volume.";
	case 0x8004280D:    // VDS_E_HIBERNATION_FILE_DISK
    	return "The operation cannot be performed on a disk that contains a hibernation file volume.";
	case 0x8004280E:    // VDS_E_CRASHDUMP_DISK
    	return "The operation cannot be performed on a disk that contains a crashdump file volume.";
	case 0x8004280F:    // VDS_E_UNABLE_TO_FIND_SYSTEM_DISK
    	return "A system error occurred while retrieving the system disk information.";
	case 0x80042810:    // VDS_E_INCORRECT_SYSTEM_VOLUME_EXTENT_INFO
    	return "Multiple disk extents reported for the system volume - system error.";
	case 0x80042811:    // VDS_E_SYSTEM_DISK
    	return "Disk attributes cannot be changed on the current system disk or BIOS disk 0.";
	case 0x80042812:    // VDS_E_VOLUME_SHRINK_FVE_LOCKED
    	return "The volume could not be shrunken because it is locked by BitLocker. Unlock the volume and try again.";
	case 0x80042813:    // VDS_E_VOLUME_SHRINK_FVE_CORRUPT
    	return "The volume could not be shrunken because it is locked due to a BitLocker error. Use BitLocker tools to recover the volume and try again.";
	case 0x80042814:    // VDS_E_VOLUME_SHRINK_FVE_RECOVERY
    	return "The volume could not be shrunken because it is marked for BitLocker recovery. Use BitLocker tools to recover the volume and try again.";
	case 0x80042815:    // VDS_E_VOLUME_SHRINK_FVE
    	return "The volume could not be shrunken because it is encrypted by BitLocker and Fveapi.dll could not be loaded to determine its status. For this operation to succeed, Fveapi.dll MUST be available in %SystemRoot%\System32\.";
	case 0x80042816:    // VDS_E_SHRINK_OVER_DATA
    	return "The SHRINK operation against the selected LUN cannot be completed. Completing the operation using the specified parameters will overwrite volumes containing user data.";
	case 0x80042817:    // VDS_E_INVALID_SHRINK_SIZE
    	return "The SHRINK operation against the selected LUN cannot be completed. The specified size is greater than the size of the LUN.";
	case 0x80042818:    // VDS_E_LUN_DISK_MISSING
    	return "The SHRINK operation against the selected LUN cannot be completed. The current state of the disk associated with the LUN is MISSING.";
	case 0x80042819:    // VDS_E_LUN_DISK_FAILED
    	return "The SHRINK operation against the selected LUN cannot be completed. The current state of the disk associated with the LUN is FAILED.";
	case 0x8004281A:    // VDS_E_LUN_DISK_NOT_READY
    	return "The SHRINK operation against the selected LUN cannot be completed. The current state of the disk associated with the LUN is NOT READY.";
	case 0x8004281B:    // VDS_E_LUN_DISK_NO_MEDIA
    	return "The SHRINK operation against the selected LUN cannot be completed. The current state of the disk associated with the LUN is NO MEDIA.";
	case 0x8004281C:    // VDS_E_LUN_NOT_READY
    	return "The SHRINK operation against the selected LUN cannot be completed. The current state of the LUN is NOT READY.";
	case 0x8004281D:    // VDS_E_LUN_OFFLINE
    	return "The SHRINK operation against the selected LUN cannot be completed. The current state of the LUN is OFFLINE.";
	case 0x8004281E:    // VDS_E_LUN_FAILED
    	return "The SHRINK operation against the selected LUN cannot be completed. The current state of the LUN is FAILED.";
	case 0x8004281F:    // VDS_E_VOLUME_EXTEND_FVE_LOCKED
    	return "The volume could not be extended because it is locked by BitLocker. Unlock the volume and retry the operation.";
	case 0x80042820:    // VDS_E_VOLUME_EXTEND_FVE_CORRUPT
    	return "The volume could not be extended because it is locked due to a BitLocker error. Use BitLocker tools to recover the volume and retry the operation.";
	case 0x80042821:    // VDS_E_VOLUME_EXTEND_FVE_RECOVERY
    	return "The volume could not be extended because it is marked for BitLocker recovery. Use BitLocker tools to recover the volume and retry the operation.";
	case 0x80042822:    // VDS_E_VOLUME_EXTEND_FVE
    	return "The volume could not be extended because it is encrypted by BitLocker and Fveapi.dll could not be loaded to determine its status. For this operation to succeed, Fveapi.dll MUST be available in %SystemRoot%\System32\.";
	case 0x80042823:    // VDS_E_SECTOR_SIZE_ERROR
    	return "The sector size MUST be non-zero, a power of 2, and less than the maximum sector size.";
	case 0x80042900:    // VDS_E_INITIATOR_ADAPTER_NOT_FOUND
    	return "The initiator adapter was not found. For calls to GetPathInfo(), the initiator adapter is associated with the path end point.";
	case 0x80042901:    // VDS_E_TARGET_PORTAL_NOT_FOUND
    	return "The target portal was not found. For calls to GetPathInfo(), the target portal is associated with the path end point.";
	case 0x80042902:    // VDS_E_INVALID_PORT_PATH
    	return "The path returned for the port is invalid. Either it has an incorrect port type specified, or, the HBA port properties structure is NULL.";
	case 0x80042903:    // VDS_E_INVALID_ISCSI_TARGET_NAME
    	return "An invalid iSCSI target name was returned from the provider.";
	case 0x80042904:    // VDS_E_SET_TUNNEL_MODE_OUTER_ADDRESS
    	return "Call to set the iSCSI tunnel mode outer address failed.";
	case 0x80042905:    // VDS_E_ISCSI_GET_IKE_INFO
    	return "Call to get the iSCSI IKE info failed.";
	case 0x80042906:    // VDS_E_ISCSI_SET_IKE_INFO
    	return "Call to set the iSCSI IKE info failed.";
	case 0x80042907:    // VDS_E_SUBSYSTEM_ID_IS_NULL
    	return "The provider returned a NULL subsystem identification string.";
	case 0x80042908:    // VDS_E_ISCSI_INITIATOR_NODE_NAME
    	return "Failed to get the iSCSI initiator node name.";
	case 0x80042909:    // VDS_E_ISCSI_GROUP_PRESHARE_KEY
    	return "Failed to set iSCSI group pre-shared key.";
	case 0x8004290A:    // VDS_E_ISCSI_CHAP_SECRET
    	return "Failed to set iSCSI initiator CHAP secret.";
	case 0x8004290B:    // VDS_E_INVALID_IP_ADDRESS
    	return "An invalid IP address was encountered.";
	case 0x8004290C:    // VDS_E_REBOOT_REQUIRED
    	return "A reboot is required before any further operations are initiated. Without a reboot, machine behavior and machine state are undefined for any further operations.";
	case 0x8004290D:    // VDS_E_VOLUME_GUID_PATHNAME_NOT_ALLOWED
    	return "Volume GUID pathnames are not valid input to this method.";
	case 0x8004290E:    // VDS_E_BOOT_PAGEFILE_DRIVE_LETTER
    	return "Assigning or removing drive letters on the current boot or pagefile volume is not allowed.";
	case 0x8004290F:    // VDS_E_DELETE_WITH_CRITICAL
    	return "Delete is not allowed on the current boot, system, pagefile, crashdump or hibernation volume.";
	case 0x80042910:    // VDS_E_CLEAN_WITH_DATA
    	return "The FORCE parameter, (see the bForce parameter in IVdsAdvancedDisk::Clean), MUST be set to TRUE in order to clean a disk that contains a data volume.";
	case 0x80042911:    // VDS_E_CLEAN_WITH_OEM
    	return "The FORCE parameter, (see the bForceOEM parameter in IVdsAdvancedDisk::Clean), MUST be set to TRUE in order to clean a disk that contains an OEM volume.";
	case 0x80042912:    // VDS_E_CLEAN_WITH_CRITICAL
    	return "Clean is not allowed on the disk containing the current boot, system, pagefile, crashdump or hibernation volume.";
	case 0x80042913:    // VDS_E_FORMAT_CRITICAL
    	return "Format is not allowed on the current boot, system, pagefile, crashdump or hibernation volume.";
	case 0x80042914:    // VDS_E_NTFS_FORMAT_NOT_SUPPORTED
    	return "The NTFS file system format is not supported on this volume.";
	case 0x80042915:    // VDS_E_FAT32_FORMAT_NOT_SUPPORTED
    	return "The FAT32 file system format is not supported on this volume.";
	case 0x80042916:    // VDS_E_FAT_FORMAT_NOT_SUPPORTED
    	return "The FAT file system format is not supported on this volume.";
	case 0x80042917:    // VDS_E_FORMAT_NOT_SUPPORTED
    	return "The volume is not formattable.";
	case 0x80042918:    // VDS_E_COMPRESSION_NOT_SUPPORTED
    	return "The specified file system does not support compression.";
	case 0x80042919:    // VDS_E_VDISK_NOT_OPEN
    	return "The virtual disk object has not been opened yet.";
	case 0x8004291A:    // VDS_E_VDISK_INVALID_OP_STATE
    	return "The requested operation cannot be performed on the virtual disk object, because it is not in a state that permits it.";
	case 0x8004291B:    // VDS_E_INVALID_PATH
    	return "The path returned for the LUN is invalid. It has an incorrect path type specified.";
	case 0x8004291C:    // VDS_E_INVALID_ISCSI_PATH
    	return "The path returned for the LUN is invalid. Either it has an incorrect path type specified, or, the initiator portal properties structure is NULL.";
	case 0x8004291D:    // VDS_E_SHRINK_LUN_NOT_UNMASKED
    	return "The SHRINK operation against the selected LUN cannot be completed. The LUN is not unmasked to the local server.";
	case 0x8004291E:    // VDS_E_LUN_DISK_READ_ONLY
    	return "The SHRINK operation against the selected LUN cannot be completed. The current state of the disk associated with the LUN is READ ONLY.";
	case 0x8004291F:    // VDS_E_LUN_UPDATE_DISK
    	return "The operation against the selected LUN completed, but there was a failure updating the status of the disk associated with the LUN. Call REFRESH to retry the status update for the disk.";
	case 0x80042920:    // VDS_E_LUN_DYNAMIC
    	return "The SHRINK operation against the selected LUN cannot be completed. The current state of the disk associated with the LUN is DYNAMIC.";
	case 0x80042921:    // VDS_E_LUN_DYNAMIC_OFFLINE
    	return "The SHRINK operation against the selected LUN cannot be completed. The current state of the disk associated with the LUN is DYNAMIC OFFLINE.";
	case 0x80042922:    // VDS_E_LUN_SHRINK_GPT_HEADER
    	return "The SHRINK operation against the selected LUN cannot be completed. The disk has the GPT partitioning format. The specified new LUN size does not allow space for a new GPT backup header to be created. Please increase the resulting LUN size.";
	case 0x80042923:    // VDS_E_MIRROR_NOT_SUPPORTED
    	return "Mirrored volumes are not supported by this operating system.";
	case 0x80042924:    // VDS_E_RAID5_NOT_SUPPORTED
    	return "RAID-5 volumes are not supported by this operating system.";
	case 0x80042925:    // VDS_E_DISK_NOT_CONVERTIBLE_SIZE
    	return "The specified disk is not convertible because the size is less than the minimum size required for GPT disks.";
	case 0x80042926:    // VDS_E_OFFLINE_NOT_SUPPORTED
    	return "The volume does not support offlining.";
	case 0x80042927:    // VDS_E_VDISK_PATHNAME_INVALID
    	return "The pathname for a virtual disk MUST be fully qualified.";
	case 0x80042928:    // VDS_E_EXTEND_TOO_MANY_CLUSTERS
    	return "The volume cannot be extended because the number of clusters will exceed the maximum number of clusters supported by the file system.";
	case 0x80042929:    // VDS_E_EXTEND_UNKNOWN_FILESYSTEM
    	return "The volume cannot be extended because the volume does not contain a recognized file system.";
	case 0x8004292A:    // VDS_E_SHRINK_UNKNOWN_FILESYSTEM
    	return "The volume cannot be shrunken because the volume does not contain a recognized file system.";
	case 0x8004292B:    // VDS_E_VD_DISK_NOT_OPEN
    	return "The requested operation requires that the virtual disk be opened.";
	case 0x8004292C:    // VDS_E_VD_DISK_IS_EXPANDING
    	return "The requested operation cannot be performed while the virtual disk is expanding.";
	case 0x8004292D:    // VDS_E_VD_DISK_IS_COMPACTING
    	return "The requested operation cannot be performed while the virtual disk is compacting.";
	case 0x8004292E:    // VDS_E_VD_DISK_IS_MERGING
    	return "The requested operation cannot be performed while the virtual disk is merging.";
	case 0x8004292F:    // VDS_E_VD_IS_ATTACHED
    	return "The requested operation cannot be performed while the virtual disk is attached.";
	case 0x80042930:    // VDS_E_VD_DISK_ALREADY_OPEN
    	return "The virtual disk is already open and cannot be opened a second time. Please close all clients that have opened the virtual disk and retry.";
	case 0x80042931:    // VDS_E_VD_DISK_ALREADY_EXPANDING
    	return "The virtual disk is already in the process of expanding.";
	case 0x80042932:    // VDS_E_VD_ALREADY_COMPACTING
    	return "The virtual disk is already in the process of compacting.";
	case 0x80042933:    // VDS_E_VD_ALREADY_MERGING
    	return "The virtual disk is already in the process of merging.";
	case 0x80042934:    // VDS_E_VD_ALREADY_ATTACHED
    	return "The virtual disk is already attached.";
	case 0x80042935:    // VDS_E_VD_ALREADY_DETACHED
    	return "The virtual disk is already detached.";
	case 0x80042936:    // VDS_E_VD_NOT_ATTACHED_READONLY
    	return "The requested operation requires that the virtual disk be attached read only.";
	case 0x80042937:    // VDS_E_VD_IS_BEING_ATTACHED
    	return "The requested operation cannot be performed while the virtual disk is being attached.";
	case 0x80042938:    // VDS_E_VD_IS_BEING_DETACHED
    	return "The requested operation cannot be performe";
	case 0x00044244:    // VDS_S_ACCESS_PATH_NOT_DELETED
    	return "The access paths on the volume is not deleted.";
	default:
		return NULL;
	}
}

static const char* GetVimError(DWORD error_code)
{
	switch (error_code) {
	case 0xC1420127:
		return "The specified image in the specified wim is already mounted for read and write access.";
	default:
		return NULL;
	}
}

#define sfree(p) do {if (p != NULL) {free((void*)(p)); p = NULL;}} while(0)
#define wconvert(p)     wchar_t* w ## p = utf8_to_wchar(p)
#define walloc(p, size) wchar_t* w ## p = (p == NULL)?NULL:(wchar_t*)calloc(size, sizeof(wchar_t))
#define wfree(p) sfree(w ## p)
#define wchar_to_utf8_no_alloc(wsrc, dest, dest_size) \
	WideCharToMultiByte(CP_UTF8, 0, wsrc, -1, dest, dest_size, NULL, NULL)

static __inline DWORD FormatMessageU(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId,
	DWORD dwLanguageId, char* lpBuffer, DWORD nSize, va_list *Arguments)
{
	DWORD ret = 0, err = ERROR_INVALID_DATA;
	// coverity[returned_null]
	walloc(lpBuffer, nSize);
	ret = FormatMessageW(dwFlags, lpSource, dwMessageId, dwLanguageId, wlpBuffer, nSize, Arguments);
	err = GetLastError();
	if ((ret != 0) && ((ret = wchar_to_utf8_no_alloc(wlpBuffer, lpBuffer, nSize)) == 0)) {
		err = GetLastError();
		ret = 0;
	}
	wfree(lpBuffer);
	SetLastError(err);
	return ret;
}


// Convert a windows error to human readable string
const char *WindowsErrorString(DWORD error_code)
{
	static char err_string[256] = { 0 };

	DWORD size, presize;
	DWORD format_error;

	// Check for VDS error codes
	if ((HRESULT_FACILITY(error_code) == FACILITY_ITF) && (GetVdsError(error_code) != NULL)) {
		sprintf_s(err_string, sizeof(err_string), "[0x%08lX] %s", error_code, GetVdsError(error_code));
		return err_string;
	}
	if ((HRESULT_FACILITY(error_code) == 322) && (GetVimError(error_code) != NULL)) {
		sprintf_s(err_string, sizeof(err_string), "[0x%08lX] %s", error_code, GetVimError(error_code));
		return err_string;
	}
	sprintf_s(err_string, sizeof(err_string), "[0x%08lX] ", error_code);
	presize = (DWORD)strlen(err_string);

	size = FormatMessageU(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
		HRESULT_CODE(error_code), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		&err_string[presize], sizeof(err_string)-(DWORD)strlen(err_string), NULL);
	if (size == 0) {
		format_error = GetLastError();
		if ((format_error) && (format_error != ERROR_MR_MID_NOT_FOUND) && (format_error != ERROR_MUI_FILE_NOT_LOADED))
			sprintf_s(err_string, sizeof(err_string), "Windows error code 0x%08lX (FormatMessage error code 0x%08lX)",
			error_code, format_error);
		else
			sprintf_s(err_string, sizeof(err_string), "Windows error code 0x%08lX", error_code);
	}
	else {
		// Microsoft may suffix CRLF to error messages, which we need to remove...		
		size += presize - 2;
		// Cannot underflow if the above assert passed since our first char is neither of the following
		while ((err_string[size] == 0x0D) || (err_string[size] == 0x0A) || (err_string[size] == 0x20))
			err_string[size--] = 0;
	}

	return err_string;
}



#define INTF_ADVANCEDDISK  1
#define INTF_ADVANCEDDISK2  2
#define INTF_CREATEPARTITIONEX  3
#define INTF_PARTITIONMF 4

/* 
 * Some code and functions in the file are copied from rufus.
 * https://github.com/pbatard/rufus
 */
#define VDS_SET_ERROR SetLastError
#define IVdsServiceLoader_LoadService(This, pwszMachineName, ppService) (This)->lpVtbl->LoadService(This, pwszMachineName, ppService)
#define IVdsServiceLoader_Release(This) (This)->lpVtbl->Release(This)
#define IVdsService_QueryProviders(This, masks, ppEnum) (This)->lpVtbl->QueryProviders(This, masks, ppEnum)
#define IVdsService_WaitForServiceReady(This) ((This)->lpVtbl->WaitForServiceReady(This))
#define IVdsService_CleanupObsoleteMountPoints(This) ((This)->lpVtbl->CleanupObsoleteMountPoints(This))
#define IVdsService_Refresh(This) ((This)->lpVtbl->Refresh(This))
#define IVdsService_Reenumerate(This) ((This)->lpVtbl->Reenumerate(This)) 
#define IVdsSwProvider_QueryInterface(This, riid, ppvObject) (This)->lpVtbl->QueryInterface(This, riid, ppvObject)
#define IVdsProvider_Release(This) (This)->lpVtbl->Release(This)
#define IVdsSwProvider_QueryPacks(This, ppEnum) (This)->lpVtbl->QueryPacks(This, ppEnum)
#define IVdsSwProvider_Release(This) (This)->lpVtbl->Release(This)
#define IVdsPack_QueryDisks(This, ppEnum) (This)->lpVtbl->QueryDisks(This, ppEnum)
#define IVdsDisk_GetProperties(This, pDiskProperties) (This)->lpVtbl->GetProperties(This, pDiskProperties)
#define IVdsDisk_Release(This) (This)->lpVtbl->Release(This)
#define IVdsDisk_QueryInterface(This, riid, ppvObject) (This)->lpVtbl->QueryInterface(This, riid, ppvObject)
#define IVdsAdvancedDisk_QueryPartitions(This, ppPartitionPropArray, plNumberOfPartitions) (This)->lpVtbl->QueryPartitions(This, ppPartitionPropArray, plNumberOfPartitions)
#define IVdsAdvancedDisk_DeletePartition(This, ullOffset, bForce, bForceProtected) (This)->lpVtbl->DeletePartition(This, ullOffset, bForce, bForceProtected)
#define IVdsAdvancedDisk_ChangeAttributes(This, ullOffset, para) (This)->lpVtbl->ChangeAttributes(This, ullOffset, para)
#define IVdsAdvancedDisk_CreatePartition(This, ullOffset, ullSize, para, ppAsync) (This)->lpVtbl->CreatePartition(This, ullOffset, ullSize, para, ppAsync)
#define IVdsAdvancedDisk_Clean(This, bForce, bForceOEM, bFullClean, ppAsync) (This)->lpVtbl->Clean(This, bForce, bForceOEM, bFullClean, ppAsync)
#define IVdsAdvancedDisk_Release(This) (This)->lpVtbl->Release(This)

#define IVdsAdvancedDisk2_ChangePartitionType(This, ullOffset, bForce, para) (This)->lpVtbl->ChangePartitionType(This, ullOffset, bForce, para)
#define IVdsAdvancedDisk2_Release(This) (This)->lpVtbl->Release(This)

#define IVdsCreatePartitionEx_CreatePartitionEx(This, ullOffset, ullSize, ulAlign, para, ppAsync) (This)->lpVtbl->CreatePartitionEx(This, ullOffset, ullSize, ulAlign, para, ppAsync)
#define IVdsCreatePartitionEx_Release(This) (This)->lpVtbl->Release(This)

#define IVdsPartitionMF_FormatPartitionEx(This, ullOffset, pwszFileSystemTypeName, usFileSystemRevision, ulDesiredUnitAllocationSize, pwszLabel, bForce, bQuickFormat, bEnableCompression, ppAsync) \
	(This)->lpVtbl->FormatPartitionEx(This, ullOffset, pwszFileSystemTypeName, usFileSystemRevision, ulDesiredUnitAllocationSize, pwszLabel, bForce, bQuickFormat, bEnableCompression, ppAsync)
#define IVdsPartitionMF_Release(This) (This)->lpVtbl->Release(This)

#define IEnumVdsObject_Next(This, celt, ppObjectArray, pcFetched) (This)->lpVtbl->Next(This, celt, ppObjectArray, pcFetched)
#define IVdsPack_QueryVolumes(This, ppEnum) (This)->lpVtbl->QueryVolumes(This, ppEnum)
#define IVdsVolume_QueryInterface(This, riid, ppvObject) (This)->lpVtbl->QueryInterface(This, riid, ppvObject)
#define IVdsVolume_Release(This) (This)->lpVtbl->Release(This)
#define IVdsVolumeMF3_QueryVolumeGuidPathnames(This, pwszPathArray, pulNumberOfPaths) (This)->lpVtbl->QueryVolumeGuidPathnames(This,pwszPathArray,pulNumberOfPaths)
#define IVdsVolumeMF3_FormatEx2(This, pwszFileSystemTypeName, usFileSystemRevision, ulDesiredUnitAllocationSize, pwszLabel, Options, ppAsync) (This)->lpVtbl->FormatEx2(This, pwszFileSystemTypeName, usFileSystemRevision, ulDesiredUnitAllocationSize, pwszLabel, Options, ppAsync)
#define IVdsVolumeMF3_Release(This) (This)->lpVtbl->Release(This)
#define IVdsVolume_GetProperties(This, pVolumeProperties) (This)->lpVtbl->GetProperties(This,pVolumeProperties)
#define IVdsAsync_Cancel(This) (This)->lpVtbl->Cancel(This)
#define IVdsAsync_QueryStatus(This,pHrResult,pulPercentCompleted) (This)->lpVtbl->QueryStatus(This,pHrResult,pulPercentCompleted)
#define IVdsAsync_Wait(This,pHrResult,pAsyncOut) (This)->lpVtbl->Wait(This,pHrResult,pAsyncOut)
#define IVdsAsync_Release(This) (This)->lpVtbl->Release(This)

#define IUnknown_QueryInterface(This, a, b) (This)->lpVtbl->QueryInterface(This,a,b)
#define IUnknown_Release(This) (This)->lpVtbl->Release(This)

typedef BOOL(*VDS_Callback_PF)(void *pInterface, VDS_DISK_PROP *pDiskProp, UINT64 data);

STATIC IVdsService * VDS_InitService(void)
{
    HRESULT hr;
    IVdsServiceLoader *pLoader;
    IVdsService *pService;

    // Initialize COM
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);

    // Create a VDS Loader Instance
    hr = CoCreateInstance(&CLSID_VdsLoader, NULL, CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER, &IID_IVdsServiceLoader, (void **)&pLoader);
    if (hr != S_OK) 
    {
        VDS_SET_ERROR(hr);
        Log("Could not create VDS Loader Instance: 0x%x", LASTERR);
        return NULL;
    }

    // Load the VDS Service
    hr = IVdsServiceLoader_LoadService(pLoader, L"", &pService);
    IVdsServiceLoader_Release(pLoader);
    if (hr != S_OK) 
    {
        VDS_SET_ERROR(hr);
        Log("Could not load VDS Service: 0x%x", LASTERR);
        return NULL;
    }

    // Wait for the Service to become ready if needed
    hr = IVdsService_WaitForServiceReady(pService);
    if (hr != S_OK) 
    {
        VDS_SET_ERROR(hr);
        Log("VDS Service is not ready: 0x%x", LASTERR);
        return NULL;
    }

    Log("VDS init OK, service %p", pService);
    return pService;
}


STATIC BOOL VDS_DiskCommProc(int intf, int DriveIndex, VDS_Callback_PF callback, UINT64 data)
{
    BOOL r = FALSE;
    HRESULT hr;
    ULONG ulFetched;
    IUnknown *pUnk = NULL;
    IEnumVdsObject *pEnum = NULL;    
    IVdsService *pService = NULL;
    wchar_t wPhysicalName[48];

    swprintf_s(wPhysicalName, ARRAYSIZE(wPhysicalName), L"\\\\?\\PhysicalDrive%d", DriveIndex);

    pService = VDS_InitService();
    if (!pService)
    {
        Log("Could not query VDS Service");
        goto out;
    }

    // Query the VDS Service Providers
    hr = IVdsService_QueryProviders(pService, VDS_QUERY_SOFTWARE_PROVIDERS, &pEnum);
    if (hr != S_OK) 
    {
        VDS_SET_ERROR(hr);
        Log("Could not query VDS Service Providers: 0x%lx %u", hr, LASTERR);
        goto out;
    }

    while (IEnumVdsObject_Next(pEnum, 1, &pUnk, &ulFetched) == S_OK) 
    {
        IVdsProvider *pProvider;
        IVdsSwProvider *pSwProvider;
        IEnumVdsObject *pEnumPack;
        IUnknown *pPackUnk;

        // Get VDS Provider
        hr = IUnknown_QueryInterface(pUnk, &IID_IVdsProvider, (void **)&pProvider);
        IUnknown_Release(pUnk);
        if (hr != S_OK) 
        {
            VDS_SET_ERROR(hr);
            Log("Could not get VDS Provider: %u", LASTERR);
            goto out;
        }

        // Get VDS Software Provider
        hr = IVdsSwProvider_QueryInterface(pProvider, &IID_IVdsSwProvider, (void **)&pSwProvider);
        IVdsProvider_Release(pProvider);
        if (hr != S_OK) 
        {
            VDS_SET_ERROR(hr);
            Log("Could not get VDS Software Provider: %u", LASTERR);
            goto out;
        }

        // Get VDS Software Provider Packs
        hr = IVdsSwProvider_QueryPacks(pSwProvider, &pEnumPack);
        IVdsSwProvider_Release(pSwProvider);
        if (hr != S_OK) 
        {
            VDS_SET_ERROR(hr);
            Log("Could not get VDS Software Provider Packs: %u", LASTERR);
            goto out;
        }

        // Enumerate Provider Packs
        while (IEnumVdsObject_Next(pEnumPack, 1, &pPackUnk, &ulFetched) == S_OK) 
        {
            IVdsPack *pPack;
            IEnumVdsObject *pEnumDisk;
            IUnknown *pDiskUnk;

            hr = IUnknown_QueryInterface(pPackUnk, &IID_IVdsPack, (void **)&pPack);
            IUnknown_Release(pPackUnk);
            if (hr != S_OK) 
            {
                VDS_SET_ERROR(hr);
                Log("Could not query VDS Software Provider Pack: %u", LASTERR);
                goto out;
            }

            // Use the pack interface to access the disks
            hr = IVdsPack_QueryDisks(pPack, &pEnumDisk);
            if (hr != S_OK) {
                VDS_SET_ERROR(hr);
                Log("Could not query VDS disks: %u", LASTERR);
                goto out;
            }

            // List disks
            while (IEnumVdsObject_Next(pEnumDisk, 1, &pDiskUnk, &ulFetched) == S_OK) 
            {
                VDS_DISK_PROP diskprop;
                IVdsDisk *pDisk;
                IVdsAdvancedDisk *pAdvancedDisk;
				IVdsAdvancedDisk2 *pAdvancedDisk2;
				IVdsCreatePartitionEx *pCreatePartitionEx;
				IVdsDiskPartitionMF *pPartitionMP;

                // Get the disk interface.
                hr = IUnknown_QueryInterface(pDiskUnk, &IID_IVdsDisk, (void **)&pDisk);
                if (hr != S_OK) {
                    VDS_SET_ERROR(hr);
                    Log("Could not query VDS Disk Interface: %u", LASTERR);
                    goto out;
                }

                // Get the disk properties
                hr = IVdsDisk_GetProperties(pDisk, &diskprop);
                if (hr != S_OK) {
                    VDS_SET_ERROR(hr);
                    Log("Could not query VDS Disk Properties: %u", LASTERR);
                    goto out;
                }

                // Isolate the disk we want
                if (_wcsicmp(wPhysicalName, diskprop.pwszName) != 0) 
                {
                    IVdsDisk_Release(pDisk);
                    continue;
                }

				if (intf == INTF_ADVANCEDDISK)
				{
					// Instantiate the AdvanceDisk interface for our disk.
					hr = IVdsDisk_QueryInterface(pDisk, &IID_IVdsAdvancedDisk, (void **)&pAdvancedDisk);
					IVdsDisk_Release(pDisk);
					if (hr != S_OK)
					{
						VDS_SET_ERROR(hr);
						Log("Could not access VDS Advanced Disk interface: %u", LASTERR);
						goto out;
					}
					else
					{
						Log("Callback %d process for disk <%S>", intf, diskprop.pwszName);
						r = callback(pAdvancedDisk, &diskprop, data);
					}
					IVdsAdvancedDisk_Release(pAdvancedDisk);
				}
				else if (intf == INTF_ADVANCEDDISK2)
				{
					// Instantiate the AdvanceDisk interface for our disk.
					hr = IVdsDisk_QueryInterface(pDisk, &IID_IVdsAdvancedDisk2, (void **)&pAdvancedDisk2);
					IVdsDisk_Release(pDisk);
					if (hr != S_OK)
					{
						VDS_SET_ERROR(hr);
						Log("Could not access VDS Advanced Disk2 interface: %u", LASTERR);
						goto out;
					}
					else
					{
						Log("Callback %d process for disk2 <%S>", intf, diskprop.pwszName);
						r = callback(pAdvancedDisk2, &diskprop, data);
					}
					IVdsAdvancedDisk2_Release(pAdvancedDisk2);
				}
				else if (intf == INTF_CREATEPARTITIONEX)
				{
					// Instantiate the CreatePartitionEx interface for our disk.
					hr = IVdsDisk_QueryInterface(pDisk, &IID_IVdsCreatePartitionEx, (void **)&pCreatePartitionEx);
					IVdsDisk_Release(pDisk);
					if (hr != S_OK)
					{
						VDS_SET_ERROR(hr);
						Log("Could not access VDS CreatePartitionEx interface: %u", LASTERR);
						goto out;
					}
					else
					{
						Log("Callback %d process for disk <%S>", intf, diskprop.pwszName);
						r = callback(pCreatePartitionEx, &diskprop, data);
					}
					IVdsCreatePartitionEx_Release(pCreatePartitionEx);
				}
				else if (intf == INTF_PARTITIONMF)
				{
					// Instantiate the DiskPartitionMF interface for our disk.
					hr = IVdsDisk_QueryInterface(pDisk, &IID_IVdsDiskPartitionMF, (void **)&pPartitionMP);
					IVdsDisk_Release(pDisk);
					if (hr != S_OK)
					{
						VDS_SET_ERROR(hr);
						Log("Could not access VDS PartitionMF interface: %u", LASTERR);
						goto out;
					}
					else
					{
						Log("Callback %d process for disk <%S>", intf, diskprop.pwszName);
						r = callback(pPartitionMP, &diskprop, data);
					}
					IVdsPartitionMF_Release(pPartitionMP);
				}

                goto out;
            }
        }
    }

out:
    return r;
}

STATIC BOOL VDS_CallBack_CleanDisk(void *pInterface, VDS_DISK_PROP *pDiskProp, UINT64 data)
{    
    HRESULT hr, hr2;
    ULONG completed;
    IVdsAsync* pAsync;
	IVdsAdvancedDisk *pAdvancedDisk = (IVdsAdvancedDisk *)pInterface;

    (void)pDiskProp;
    (void)data;

    hr = IVdsAdvancedDisk_Clean(pAdvancedDisk, TRUE, TRUE, FALSE, &pAsync);
    while (SUCCEEDED(hr)) 
    {
        hr = IVdsAsync_QueryStatus(pAsync, &hr2, &completed);
        if (SUCCEEDED(hr)) 
        {
            hr = hr2;
            if (hr == S_OK)
            {
                Log("Disk clean QueryStatus OK");
                break;
            }
            else if (hr == VDS_E_OPERATION_PENDING)
            {
                hr = S_OK;
            }
            else
            {
                Log("QueryStatus invalid status:%u", hr);
            }
        }
        Sleep(500);
    }

    if (hr != S_OK) 
    {
        VDS_SET_ERROR(hr);
        Log("Could not clean disk 0x%lx err:%u", hr, LASTERR);
        return FALSE;
    }

    return TRUE;
}

BOOL VDS_CleanDisk(int DriveIndex)
{
	BOOL ret = VDS_DiskCommProc(INTF_ADVANCEDDISK, DriveIndex, VDS_CallBack_CleanDisk, 0);
    Log("VDS_CleanDisk %d ret:%d (%s)", DriveIndex, ret, ret ? "SUCCESS" : "FAIL");
    return ret;
}

STATIC BOOL VDS_CallBack_DeletePartition(void *pInterface, VDS_DISK_PROP *pDiskProp, UINT64 data)
{
    BOOL r = FALSE;
    HRESULT hr;
    VDS_PARTITION_PROP* prop_array = NULL;
    LONG i, prop_array_size;
    ULONG PartNumber = (ULONG)data;
	IVdsAdvancedDisk *pAdvancedDisk = (IVdsAdvancedDisk *)pInterface;

    if (PartNumber == 0)
    {
        Log("Deleting ALL partitions from disk '%S':", pDiskProp->pwszName);
    }
    else
    {
		Log("Deleting partition(%ld) from disk '%S':", PartNumber, pDiskProp->pwszName);
    }

    // Query the partition data, so we can get the start offset, which we need for deletion
    hr = IVdsAdvancedDisk_QueryPartitions(pAdvancedDisk, &prop_array, &prop_array_size);
    if (hr == S_OK) 
    {
		r = TRUE;
        for (i = 0; i < prop_array_size; i++) 
        {
            if (PartNumber == 0 || PartNumber == prop_array[i].ulPartitionNumber)
            {
                Log("* Partition %d (offset: %lld, size: %llu) delete it.",
                    prop_array[i].ulPartitionNumber, prop_array[i].ullOffset, (ULONGLONG)prop_array[i].ullSize);
            }
            else
            {
                Log("  Partition %d (offset: %lld, size: %llu) skip it.",
                    prop_array[i].ulPartitionNumber, prop_array[i].ullOffset, (ULONGLONG)prop_array[i].ullSize);
                continue;
            }

            hr = IVdsAdvancedDisk_DeletePartition(pAdvancedDisk, prop_array[i].ullOffset, TRUE, TRUE);
            if (hr != S_OK) 
            {
                r = FALSE;
                VDS_SET_ERROR(hr);
                Log("Could not delete partitions: 0x%x", LASTERR);
				break;
            }
            else 
            {
                Log("Delete this partitions success");
            }
        }
    }
    else 
    {
        Log("No partition to delete on disk '%S'", pDiskProp->pwszName);
        r = TRUE;
    }

	if (prop_array)
	{
		CoTaskMemFree(prop_array);
	}

    return r;
}

BOOL VDS_DeleteAllPartitions(int DriveIndex)
{
	BOOL ret = VDS_DiskCommProc(INTF_ADVANCEDDISK, DriveIndex, VDS_CallBack_DeletePartition, 0);    
	Log("VDS_DeleteAllPartitions %d ret:%d (%s)", DriveIndex, ret, ret ? "SUCCESS" : "FAIL");
    return ret;
}

BOOL VDS_DeleteVtoyEFIPartition(int DriveIndex)
{
	BOOL ret = VDS_DiskCommProc(INTF_ADVANCEDDISK, DriveIndex, VDS_CallBack_DeletePartition, 2);
	Log("VDS_DeleteVtoyEFIPartition %d ret:%d (%s)", DriveIndex, ret, ret ? "SUCCESS" : "FAIL");
    return ret;
}

STATIC BOOL VDS_CallBack_ChangeEFIAttr(void *pInterface, VDS_DISK_PROP *pDiskProp, UINT64 data)
{
    BOOL r = FALSE;
    HRESULT hr;
    VDS_PARTITION_PROP* prop_array = NULL;
    LONG i, prop_array_size;
    CHANGE_ATTRIBUTES_PARAMETERS AttrPara;
	IVdsAdvancedDisk *pAdvancedDisk = (IVdsAdvancedDisk *)pInterface;

    // Query the partition data, so we can get the start offset, which we need for deletion
    hr = IVdsAdvancedDisk_QueryPartitions(pAdvancedDisk, &prop_array, &prop_array_size);
    if (hr == S_OK)
    {
        for (i = 0; i < prop_array_size; i++)
        {
            if (prop_array[i].ullSize == VENTOY_EFI_PART_SIZE &&
                prop_array[i].PartitionStyle == VDS_PST_GPT &&
                memcmp(prop_array[i].Gpt.name, L"VTOYEFI", 7 * 2) == 0)
            {
                Log("* Partition %d (offset: %lld, size: %llu, Attr:0x%llx)", prop_array[i].ulPartitionNumber,
                    prop_array[i].ullOffset, (ULONGLONG)prop_array[i].ullSize, prop_array[i].Gpt.attributes);

                if (prop_array[i].Gpt.attributes == data)
                {
                    Log("Attribute match, No need to change.");
                    r = TRUE;
                }
                else
                {
                    AttrPara.style = VDS_PST_GPT;
                    AttrPara.GptPartInfo.attributes = data;
                    hr = IVdsAdvancedDisk_ChangeAttributes(pAdvancedDisk, prop_array[i].ullOffset, &AttrPara);
                    if (hr == S_OK)
                    {
                        r = TRUE;
                        Log("Change this partitions attribute success");
                    }
                    else
                    {
                        r = FALSE;
                        VDS_SET_ERROR(hr);
                        Log("Could not change partitions attr: %u", LASTERR);
                    }
                }
                break;
            }
        }
    }
    else
    {
        Log("No partition found on disk '%S'", pDiskProp->pwszName);
    }
    CoTaskMemFree(prop_array);

    return r;
}

BOOL VDS_ChangeVtoyEFIAttr(int DriveIndex, UINT64 Attr)
{
	BOOL ret = VDS_DiskCommProc(INTF_ADVANCEDDISK, DriveIndex, VDS_CallBack_ChangeEFIAttr, Attr);    
	Log("VDS_ChangeVtoyEFIAttr %d ret:%d (%s)", DriveIndex, ret, ret ? "SUCCESS" : "FAIL");
    return ret;
}



STATIC BOOL VDS_CallBack_ChangeEFIType(void *pInterface, VDS_DISK_PROP *pDiskProp, UINT64 data)
{
	BOOL r = FALSE;
	HRESULT hr;
	IVdsAdvancedDisk2 *pAdvancedDisk2 = (IVdsAdvancedDisk2 *)pInterface;
	VDS_PARA *VdsPara = (VDS_PARA *)(ULONG)data;
	CHANGE_PARTITION_TYPE_PARAMETERS para;

	para.style = VDS_PST_GPT;
	memcpy(&(para.GptPartInfo.partitionType), &VdsPara->Type, sizeof(GUID));

	hr = IVdsAdvancedDisk2_ChangePartitionType(pAdvancedDisk2, VdsPara->Offset, TRUE, &para);
	if (hr == S_OK)
	{
		r = TRUE;
	}
	else
	{
		Log("Failed to change partition type 0x%lx(%s)", hr, WindowsErrorString(hr));
	}

	return r;
}


BOOL VDS_ChangeVtoyEFI2ESP(int DriveIndex, UINT64 Offset)
{
	VDS_PARA Para;	
	GUID EspPartType = { 0xc12a7328, 0xf81f, 0x11d2, { 0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b } };

	memcpy(&(Para.Type), &EspPartType, sizeof(GUID));
	Para.Offset = Offset;

	BOOL ret = VDS_DiskCommProc(INTF_ADVANCEDDISK2, DriveIndex, VDS_CallBack_ChangeEFIType, (ULONG)&Para);
	Log("VDS_ChangeVtoyEFI2ESP %d ret:%d (%s)", DriveIndex, ret, ret ? "SUCCESS" : "FAIL");
	return ret;
}

BOOL VDS_ChangeVtoyEFI2Basic(int DriveIndex, UINT64 Offset)
{
	VDS_PARA Para;
	GUID WindowsDataPartType = { 0xebd0a0a2, 0xb9e5, 0x4433, { 0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7 } };

	memcpy(&(Para.Type), &WindowsDataPartType, sizeof(GUID));
	Para.Offset = Offset;

	BOOL ret = VDS_DiskCommProc(INTF_ADVANCEDDISK2, DriveIndex, VDS_CallBack_ChangeEFIType, (ULONG)&Para);
	Log("VDS_ChangeVtoyEFI2Basic %d ret:%d (%s)", DriveIndex, ret, ret ? "SUCCESS" : "FAIL");
	return ret;
}


STATIC BOOL VDS_CallBack_CreateVtoyEFI(void *pInterface, VDS_DISK_PROP *pDiskProp, UINT64 data)
{
    HRESULT hr, hr2;
    ULONG completed;
    IVdsAsync* pAsync;
	CREATE_PARTITION_PARAMETERS para;
	IVdsCreatePartitionEx *pCreatePartitionEx = (IVdsCreatePartitionEx *)pInterface;
    VDS_PARA *VdsPara = (VDS_PARA *)(ULONG)data;

    (void)pDiskProp;

    memset(&para, 0, sizeof(para));
    para.style = VDS_PST_GPT;
    memcpy(&(para.GptPartInfo.partitionType), &VdsPara->Type, sizeof(GUID));
    memcpy(&(para.GptPartInfo.partitionId), &VdsPara->Id, sizeof(GUID));
	para.GptPartInfo.attributes = VdsPara->Attr;
	memcpy(para.GptPartInfo.name, VdsPara->Name, sizeof(WCHAR)* VdsPara->NameLen);

	hr = IVdsCreatePartitionEx_CreatePartitionEx(pCreatePartitionEx, VdsPara->Offset, VENTOY_EFI_PART_SIZE, 512, &para, &pAsync);
    while (SUCCEEDED(hr))
    {
        hr = IVdsAsync_QueryStatus(pAsync, &hr2, &completed);
        if (SUCCEEDED(hr))
        {
            hr = hr2;
            if (hr == S_OK)
            {
				Log("Disk create partition QueryStatus OK, %lu%%", completed);
                break;
            }
            else if (hr == VDS_E_OPERATION_PENDING)
            {
				Log("Disk partition finish: %lu%%", completed);
                hr = S_OK;
            }
            else
            {
                Log("QueryStatus invalid status:0x%lx", hr);
            }
        }
        Sleep(1000);
    }

    if (hr != S_OK)
    {
        VDS_SET_ERROR(hr);
		Log("Could not create partition, err:0x%lx(%s)", LASTERR, WindowsErrorString(hr));
        return FALSE;
    }

    return TRUE;
}

BOOL VDS_CreateVtoyEFIPart(int DriveIndex, UINT64 Offset)
{
    VDS_PARA Para;
    GUID WindowsDataPartType = { 0xebd0a0a2, 0xb9e5, 0x4433, { 0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7 } };
	GUID EspPartType = { 0xc12a7328, 0xf81f, 0x11d2, { 0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b } };

	Log("VDS_CreateVtoyEFIPart %u Offset:%llu Sector:%llu", DriveIndex, Offset, Offset / 512);

    memset(&Para, 0, sizeof(Para));
    Para.Attr = 0x8000000000000000ULL;
    Para.Offset = Offset;
    memcpy(Para.Name, L"VTOYEFI", 7 * 2);
	Para.NameLen = 7;
	memcpy(&(Para.Type), &EspPartType, sizeof(GUID));
    CoCreateGuid(&(Para.Id));

	BOOL ret = VDS_DiskCommProc(INTF_CREATEPARTITIONEX, DriveIndex, VDS_CallBack_CreateVtoyEFI, (ULONG)&Para);    
	Log("VDS_CreateVtoyEFIPart %d ret:%d (%s)", DriveIndex, ret, ret ? "SUCCESS" : "FAIL");
    return ret; 
}


STATIC BOOL VDS_CallBack_FormatVtoyEFI(void *pInterface, VDS_DISK_PROP *pDiskProp, UINT64 data)
{
	HRESULT hr, hr2;
	ULONG completed;
	IVdsAsync* pAsync;	
	IVdsDiskPartitionMF *pPartitionMF = (IVdsDiskPartitionMF *)pInterface;
	VDS_PARA *VdsPara = (VDS_PARA *)(ULONG)data;

	(void)pDiskProp;
	
	hr = IVdsPartitionMF_FormatPartitionEx(pPartitionMF, VdsPara->Offset, L"FAT", 0x0100, 0, VdsPara->Name, TRUE, TRUE, FALSE, &pAsync);
	while (SUCCEEDED(hr))
	{
		hr = IVdsAsync_QueryStatus(pAsync, &hr2, &completed);
		if (SUCCEEDED(hr))
		{
			hr = hr2;
			if (hr == S_OK)
			{
				Log("Disk format partition QueryStatus OK, %lu%%", completed);
				break;
			}
			else if (hr == VDS_E_OPERATION_PENDING)
			{
				Log("Disk format finish: %lu%%", completed);
				hr = S_OK;
			}
			else
			{
				Log("QueryStatus invalid status:0x%lx", hr);
			}
		}
		Sleep(1000);
	}

	if (hr != S_OK)
	{
		VDS_SET_ERROR(hr);
		Log("Could not format partition, err:0x%lx (%s)", LASTERR, WindowsErrorString(hr));
		return FALSE;
	}

	return TRUE;
}

// Not supported for removable disk
BOOL VDS_FormatVtoyEFIPart(int DriveIndex, UINT64 Offset)
{
	VDS_PARA Para;

	memset(&Para, 0, sizeof(Para));
	Para.Offset = Offset;
	memcpy(Para.Name, L"VTOYEFI", 7 * 2);

	BOOL ret = VDS_DiskCommProc(INTF_PARTITIONMF, DriveIndex, VDS_CallBack_FormatVtoyEFI, (ULONG)&Para);	
	Log("VDS_FormatVtoyEFIPart %d ret:%d (%s)", DriveIndex, ret, ret ? "SUCCESS" : "FAIL");
	return ret;
}

