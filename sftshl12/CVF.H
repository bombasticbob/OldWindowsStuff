/***    CVF.H - Definitions for the DoubleSpace Compressed Volume File
 *
 *      Version 1.00.53
 */

#pragma pack(1)

typedef unsigned char  BYTE;    // 1 byte
typedef unsigned short WORD;    // 2 bytes
typedef unsigned long  DWORD;   // 4 bytes


/***    szDS_STAMP1, szDS_STAMP2 - Stamps that identify a DoubleSpace CVF
 *
 *      These two stamps are written, respectively, at the following
 *      two locations in the CVF:
 *        1) csecMDReserved*cbPerSec+512
 *        2) Start of last sector of CVF
 *
 *      The first stamp starts with F8, to be compatible with Colorado
 *      Memory Systems tape backup software.
 *
 */
#define szDS_STAMP1  "\xf8" "DR" // First CVF stamp
#define szDS_STAMP2  "MDR"      // Second CVF stamp
#define cbDS_STAMP   4          // Length of stamp (includes NULL)


/***    csecRESERVED1 - count of sectors in CVF region RESERVED1
 *
 *      This is the region between the BitFAT and the MDFAT.
 */
#define csecRESERVED1  1        // Hard-coded size of reserved region 1


/***    csecRESERVED2 - count of sectors in CVF region RESERVED2
 *
 *      This is the region between the MDFAT and the DOS BOOT region.
 */
#define csecRESERVED2 31        // Hard-coded size of reserved region 2


/***    csecRESERVED4 - count of sectors in CVF region RESERVED4
 *
 *      This is the region between the ROOTDIR and the SECTORHEAP.
 */
#define csecRESERVED4  2        // Hard-coded size of reserved region 4


/***    csecRETRACT_STAMP - count of sectors at end of Sector Heap
 *
 *      The tail CVF stamp is stored in the last *complete* sector
 *      of the CVF.  That is, if the CVF is exactly a multiple of 512
 *      bytes in length, then the last sector contains the stamp.
 *      If the CVF is *not* a multiple of 512 bytes, then the stamp
 *      is stored in the next to last sector in the file, which is
 *      512 bytes long (the last sector is less than 512 bytes).
 */
#define csecRETRACT_STAMP   1


/***    cbPER_BITFAT_PAGE - size of a BitFAT page in DBLSPACE.BIN
 *
 *      DBLSPACE.BIN "pages" the BitFAT into a buffer of this size,
 *      so the BitFAT total size must be a multiple of this page size.
 */
#define cbPER_BITFAT_PAGE   2048


/***    MDBPB - Compressed Volume File "BIOS Parameter Block"
 *
 *      This structure appears in the first sector of a Compressed
 *      Volume File, and contains information that describes the
 *      size and location of the remaining parts of the CVF.
 */

typedef struct { /* mp */

    BYTE    jmpBOOT[3];     // Jump to bootstrap routine
    char    achOEMName[8];  // OEM Name ("MSDBL6.0")

    /*
     *  The following fields are a clone of an MS-DOS BPB
     */
    WORD    cbPerSec;       // Count of bytes per sector (always 512)
    BYTE    csecPerClu;     // Count of sectors per cluster (always 16)
    WORD    csecReserved;   // Count of reserved sectors.
    BYTE    cFATs;          // Count of FATs (always 1)
                            // NOTE: Even though we store 1 on the disk,
                            //       when the drive is mounted, DBLSPACE.BIN
                            //       simulates 2 FATs.
    WORD    cRootDirEntries;// Count of root directory entries (always 512)
    WORD    csecTotalWORD;  // Count of total sectors (see csecTotalDWORD if 0)
    BYTE    bMedia;         // Media byte (always 0xF8 == hard disk)
    WORD    csecFAT;        // Count of sectors occupied by the FAT
    WORD    csecPerTrack;   // Count of sectors per track (random, ignored)
    WORD    cHeads;         // Count of heads (random, ignored)
    DWORD   csecHidden;     // Count of hidden sectors
    DWORD   csecTotalDWORD; // Count of total sectors

    /*
     *  The following fields are DoubleSpace extensions
     */

    WORD    secMDFATStart;  // Logical sector of start of MDFAT
    BYTE    nLog2cbPerSec;  // Log base 2 of cbPerSec
    WORD    csecMDReserved; // Number of sectors before DOS BOOT sector
    WORD    secRootDirStart;// Logical sector of start of root directory
    WORD    secHeapStart;   // Logical sector of start of sector heap
    WORD    cluFirstData;   // Number of MDFAT entries (clusters) which are
                            // occupied by the DOS boot sector, reserved area,
                            // and root directory.
    BYTE    cpageBitFAT;    // Count of 2K pages in the BitFAT
    WORD    RESERVED1;
    BYTE    nLog2csecPerClu;// Log base 2 of csecPerClu
    WORD    RESERVED2;
    DWORD   RESERVED3;
    DWORD   RESERVED4;
    BYTE    f12BitFAT;      // 1 => 12-bit FAT, 0 => 16-bit FAT
    WORD    cmbCVFMax;      // Maximum CVF size, in megabytes (1024*1024)
} MDBPB;
typedef MDBPB *PMDBPB;  /* pmp */


/***    MDFATENTRY - Entry in the MDFAT
 *
 *      The MDFAT is a table that is used to keep track of the data for
 *      each cluster in the DoubleSpace drive.
 *
 *      The MDFAT parallels the DOS FAT in the CVF.
 *
 *      In a normal FAT partition, the starting sector for a cluster is found
 *      by this formula:
 *
 *          sector  = FAT overhead + cluster number * sectors per cluster
 *
 *      This formula is simple, and allows the FAT to be used both as a
 *      free space map and also as a means of linking the clusters of a
 *      file together.  However, it also means that the physical space
 *      occupied by a file is a multiple of the cluster size.
 *
 *      In a CVF, on the other hand, this mapping does not apply.  The DOS
 *      FAT is still used by MS-DOS for "virtual" space allocation and linking
 *      file clusters, but no physical space allocation in the CVF occurs
 *      until disk I/O occurs on the data of a cluster.
 *
 *      To find the data for a cluster in a CVF, the cluster number is used
 *      as an index into the MDFAT, and the MDFAT Entry describes the state,
 *      location, and size of the cluster, as follows:
 *
 *      secStart
 *          This is the logical sector in the CVF Sector Heap where the
 *          data for the cluster resides.  All the sectors of this cluster
 *          are stored contiguously, starting at this location.
 *          Add 1 to this value to get the logical sector number (from the
 *          beginning of the CVF), i.e., if secStart is 134, then the sector
 *          is located at sector 135 in the CVF.
 *
 *      csecCoded
 *          This is the length of the cluster data, in sectors.
 *          Values 0..15 are interpreted as 1..16.
 *          A maximally compressed cluster will have length 0.  An
 *          uncompressed cluster will have the length MDBPB.csecPerClu-1 (15).
 *
 *      csecPlain
 *          This is the length of the uncompressed cluster data, in sectors.
 *          Values are 0..15, as with csecCoded.  This value is usually
 *          MDBPB.csecPerClu-1 (15), except for the last cluster of a file,
 *          which may be shorter, since the cluster has not been completely
 *          written.
 *
 *      fUncoded
 *          Indicates whether the cluster data is compressed.
 *          1 => uncompressed; 0 => compressed.  If DoubleSpace is unable
 *          to achieve at least a 1 sector saving, data is stored uncompressed.
 *
 *      fUsed
 *          Indicates whether this entry is in use.  This is used to allow
 *          DoubleSpace to support FAT "undelete" programs.  DBLSPACE.BIN scans
 *          a FAT sector when it is written to disk to infer three types
 *          of operations:
 *
 *          1) Cluster allocation by DOS
 *          2) Cluster free (by DOS or a utility program, like a defragger)
 *          3) Cluster "resurrection" by an undelete program.
 */
typedef struct { /* mfe */
    unsigned long secStart       : 22; // Starting sector in sector heap
    unsigned int  csecCoded	 :  4; // Length of coded data (in sectors)
    unsigned int  csecPlain	 :  4; // Length of original data (in sectors)
    unsigned int  fUncoded	 :  1; // TRUE => coded data is NOT CODED
    unsigned int  fUsed 	 :  1; // TRUE => MDFAT entry is allocate
} MDFATENTRY;
typedef MDFATENTRY *PMDFATENTRY; /* pmfe */

// NOTE: C6 does not want to treat MDFATENTRY as a 4 byte structre, so
//       we hard-code the length here, and use explicit masks and shifts
//       in code that manipulates MDFATENTRYs.

#define cbMDFATENTRY   4
