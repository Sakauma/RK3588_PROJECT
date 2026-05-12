param(
    [Parameter(Mandatory = $true)]
    [string]$InputDir,

    [string]$OutputDir = "",

    [int]$Width = 2048,

    [int]$Height = 2048,

    [int]$Limit = 0,

    [switch]$Force
)

if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = Join-Path (Split-Path -Parent $InputDir) "gray10le16"
}

if (-not (Test-Path -LiteralPath $InputDir)) {
    throw "InputDir does not exist: $InputDir"
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

Add-Type -ReferencedAssemblies "System.Drawing" -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;

public static class Gray10Le16Converter
{
    public static void ConvertPng(string inputPath, string outputPath, int width, int height)
    {
        using (Bitmap source = (Bitmap)Image.FromFile(inputPath))
        {
            if (source.Width != width || source.Height != height)
            {
                throw new InvalidOperationException(
                    String.Format("Unexpected resolution {0}x{1}, expected {2}x{3}",
                        source.Width, source.Height, width, height));
            }

            using (Bitmap rgb = new Bitmap(width, height, PixelFormat.Format24bppRgb))
            {
                using (Graphics graphics = Graphics.FromImage(rgb))
                {
                    graphics.DrawImage(source, 0, 0, width, height);
                }

                Rectangle rect = new Rectangle(0, 0, width, height);
                BitmapData data = rgb.LockBits(rect, ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);
                try
                {
                    int stride = Math.Abs(data.Stride);
                    byte[] src = new byte[stride * height];
                    Marshal.Copy(data.Scan0, src, 0, src.Length);

                    byte[] dst = new byte[width * height * 2];
                    int dstIndex = 0;
                    for (int y = 0; y < height; y++)
                    {
                        int row = data.Stride >= 0 ? y * stride : (height - 1 - y) * stride;
                        for (int x = 0; x < width; x++)
                        {
                            int srcIndex = row + x * 3;
                            int b = src[srcIndex + 0];
                            int g = src[srcIndex + 1];
                            int r = src[srcIndex + 2];
                            int gray8 = (77 * r + 150 * g + 29 * b + 128) >> 8;

                            // gray8 -> gray10 = gray8 << 2; FPGA raw16 container = gray10 << 6.
                            // The resulting little-endian 16-bit sample is gray8 << 8.
                            dst[dstIndex++] = 0;
                            dst[dstIndex++] = (byte)gray8;
                        }
                    }

                    File.WriteAllBytes(outputPath, dst);
                }
                finally
                {
                    rgb.UnlockBits(data);
                }
            }
        }
    }
}
"@

$files = Get-ChildItem -LiteralPath $InputDir -File |
    Where-Object { $_.Extension -match '^\.(png|bmp|jpg|jpeg)$' } |
    Sort-Object Name

if ($Limit -gt 0) {
    $files = $files | Select-Object -First $Limit
}

if (-not $files -or $files.Count -eq 0) {
    throw "No image files found in $InputDir"
}

$expectedBytes = $Width * $Height * 2
$converted = 0
foreach ($file in $files) {
    $outName = [System.IO.Path]::GetFileNameWithoutExtension($file.Name) + ".gray10le16"
    $outPath = Join-Path $OutputDir $outName

    if ((Test-Path -LiteralPath $outPath) -and -not $Force) {
        Write-Host "skip existing $outPath"
        continue
    }

    [Gray10Le16Converter]::ConvertPng($file.FullName, $outPath, $Width, $Height)
    $length = (Get-Item -LiteralPath $outPath).Length
    if ($length -ne $expectedBytes) {
        throw "Output size mismatch for $outPath actual=$length expected=$expectedBytes"
    }

    $converted++
    Write-Host ("converted {0} -> {1} ({2} bytes)" -f $file.Name, $outName, $length)
}

Write-Host ("done: converted={0}, output={1}, frame_bytes={2}" -f $converted, $OutputDir, $expectedBytes)
