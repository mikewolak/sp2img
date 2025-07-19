# SP to IMG Converter

A utility for converting Peavey SP sampler disk images from the proprietary `.sp` format to standard `.img` format compatible with GreaseWeazle and other modern disk imaging tools.

## Author

**Mike Wolak** - mikewolak@gmail.com

## Purpose

This tool bridges vintage and modern disk imaging workflows by converting disk images created with the original DOS-based SPREAD.C utility (circa 1996) to a format that can be written to physical floppies using contemporary hardware like the GreaseWeazle USB floppy controller.

The original SPREAD.C and SPWRITE.C programs were designed for Peavey SP sampler users to backup and restore their sample libraries, but required vintage DOS systems with direct BIOS floppy access. This converter enables the same disk images to be used with modern, cross-platform imaging hardware.

## Peavey SP Sampler Disk Format

Peavey SP samplers used standard 3.5" HD floppy disks with the following specifications:

- **Capacity**: 1,440 KB (1,474,560 bytes)
- **Tracks per side**: 80 (numbered 0-79)
- **Heads**: 2 (sides 0 and 1)
- **Sectors per track**: 18 (numbered 1-18)
- **Bytes per sector**: 512
- **Encoding**: IBM MFM (Modified Frequency Modulation)
- **Data rate**: 500 kbps (HD rate)

### Total disk geometry:
- **2 heads × 80 tracks × 18 sectors × 512 bytes = 1,474,560 bytes**

## Disk Layout Conversion

### .SP Format (SPREAD.C Output)

The original SPREAD.C utility reads tracks sequentially, alternating between heads for each cylinder:

```
Track 0, Head 0  (18 sectors × 512 bytes = 9,216 bytes)
Track 0, Head 1  (18 sectors × 512 bytes = 9,216 bytes)
Track 1, Head 0  (18 sectors × 512 bytes = 9,216 bytes)
Track 1, Head 1  (18 sectors × 512 bytes = 9,216 bytes)
Track 2, Head 0  (18 sectors × 512 bytes = 9,216 bytes)
Track 2, Head 1  (18 sectors × 512 bytes = 9,216 bytes)
...
Track 79, Head 0 (18 sectors × 512 bytes = 9,216 bytes)
Track 79, Head 1 (18 sectors × 512 bytes = 9,216 bytes)
```

**File offset calculation for .SP format:**
- Track N, Head 0: `offset = N × 2 × 9216`
- Track N, Head 1: `offset = N × 2 × 9216 + 9216`

### .IMG Format (Standard)

The standard .IMG format groups all tracks by head:

```
Track 0, Head 0   (18 sectors × 512 bytes = 9,216 bytes)
Track 1, Head 0   (18 sectors × 512 bytes = 9,216 bytes)
Track 2, Head 0   (18 sectors × 512 bytes = 9,216 bytes)
...
Track 79, Head 0  (18 sectors × 512 bytes = 9,216 bytes)
Track 0, Head 1   (18 sectors × 512 bytes = 9,216 bytes)
Track 1, Head 1   (18 sectors × 512 bytes = 9,216 bytes)
Track 2, Head 1   (18 sectors × 512 bytes = 9,216 bytes)
...
Track 79, Head 1  (18 sectors × 512 bytes = 9,216 bytes)
```

**File offset calculation for .IMG format:**
- Track N, Head 0: `offset = N × 9216`
- Track N, Head 1: `offset = (80 × 9216) + (N × 9216)`

### Conversion Process

The `sp2img` utility performs the layout conversion in two phases:

1. **Phase 1**: Extract all Head 0 tracks from the .SP file and write them sequentially
2. **Phase 2**: Extract all Head 1 tracks from the .SP file and append them

This rearrangement maintains all sector data while conforming to the standard .IMG layout expected by modern tools.

## Building and Usage

### Compilation

```bash
# Simple build
make sp2img

# Debug version
make debug

# Static binary (portable)
make static

# Windows cross-compile (requires mingw)
make windows
```

### Usage

```bash
# Convert single file
./sp2img input.sp output.img

# Batch convert directory
./batch_convert.sh /path/to/sp_files/

# Write to floppy with GreaseWeazle
gw write --format=ibm.1440 output.img
```

## GreaseWeazle Integration

### Method 1: Standard IMG Format (Recommended)

After conversion, use standard GreaseWeazle commands:

```bash
# Write disk image
gw write --format=ibm.1440 --verify sample_disk.img

# Specify drive if multiple connected
gw write --format=ibm.1440 --drive=0 sample_disk.img

# Read verification
gw read --format=ibm.1440 verify_read.img
```

### Method 2: Custom Disk Definition (Alternative)

If you prefer to work directly with .SP files or need to customize the disk format, you can create a custom disk definition.

#### Adding Custom Disk Definition

1. **Locate your GreaseWeazle diskdefs file:**
   - Windows: Usually in the same directory as `gw.exe`
   - Linux/macOS: `~/.config/greaseweazle/diskdefs.cfg` or system-wide location

2. **Add the Peavey SP definition to `diskdefs.cfg`:**

```cfg
# Peavey SP Sampler Disk Format
disk peavey.sp
    cyls = 80
    heads = 2
    tracks * ibm.mfm
        secs = 18
        bps = 512
        rate = 500
        iam = yes
        id = 1
        interleave = 1
        cskew = 0
        hskew = 0
        gap3 = 84
    end
end

# Alternative definition if timing issues occur
disk peavey.sp.alt
    cyls = 80
    heads = 2  
    tracks * ibm.mfm
        secs = 18
        bps = 512
        rate = 500
        iam = yes
        id = 1
        interleave = 1
        cskew = 1     # Cylinder skew
        hskew = 3     # Head skew
        gap3 = 54     # Tighter gap
    end
end
```

3. **Using the custom definition:**

```bash
# Read with custom format
gw read --format=peavey.sp original_disk.sp

# Write with custom format (after converting .sp to proper layout)
gw write --format=peavey.sp converted_disk.img
```

#### Disk Definition Parameters Explained

- **`cyls = 80`**: 80 cylinders (tracks 0-79)
- **`heads = 2`**: Double-sided disk
- **`secs = 18`**: 18 sectors per track
- **`bps = 512`**: 512 bytes per sector
- **`rate = 500`**: 500 kbps data rate (HD)
- **`iam = yes`**: Include Index Address Mark
- **`id = 1`**: First logical sector ID (standard)
- **`interleave = 1`**: 1:1 sector interleave (sequential)
- **`cskew = 0`**: No cylinder-to-cylinder skew
- **`hskew = 0`**: No head-to-head skew
- **`gap3 = 84`**: Post-data gap size

### Troubleshooting Custom Definitions

If the standard definition doesn't work perfectly:

1. **Try the alternative definition** with different skew values
2. **Adjust gap sizes** - some drives are picky about timing
3. **Verify sector numbering** - check if sectors start at 0 instead of 1
4. **Test data rate** - some samplers used non-standard rates

## File Verification

To verify conversion accuracy:

```bash
# Check file sizes
ls -l original.sp converted.img
# Both should be exactly 1,474,560 bytes

# Compare checksums after conversion back
gw read --format=ibm.1440 verification.img
cmp converted.img verification.img
```

## Compatibility

- **GreaseWeazle**: All versions (F1, F7, V4.x)
- **FluxEngine**: Compatible via .IMG format
- **HxC Floppy Emulator**: Can convert .IMG to other formats
- **Gotek/FlashFloppy**: Direct .IMG support
- **Vintage Systems**: Images work with original Peavey SP samplers

## License

This tool is provided as-is for preserving vintage sampler libraries. Use responsibly and respect copyright of any commercial sample content.

## Related Tools

- **SPREAD.C / SPWRITE.C**: Original DOS imaging utilities by Vincent Raue (1996)
- **GreaseWeazle**: Modern USB floppy controller by Keir Fraser
- **HxC Floppy Emulator**: Swiss Army knife of disk image conversion

## Support

For technical issues or questions about this converter, contact Mike Wolak at mikewolak@gmail.com.

For GreaseWeazle-specific questions, consult the official documentation at: https://github.com/keirf/greaseweazle/wiki
