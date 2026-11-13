#!/usr/bin/env python3
"""Makes an effect bundle (.rfx): one effect compiled for every graphics API of Raven, in one file.

    rfx.py wave.frag -o wave.rfx                        from the GLSL of an effect (raven_effect.glsl)
    rfx.py wave.frag -o wave.rfx --backdrop             the effect reads what is behind it (R_SHADER_BACKDROP)
    rfx.py wave.spv  -o wave.rfx                        from SPIR-V already compiled (needs no glslang)

Then Rshader_create_from_file("wave.rfx", 0) or <Border Effect="wave.rfx"/> works with the Vulkan backend (which
takes the SPIR-V chunk) and the Sokol backend (GLES 3 / WebGL 2 and Metal: their source chunks), on every platform.
The format is described in src/render/rfx.h.

Needs glslangValidator (unless the input is SPIR-V) and spirv-cross on the PATH, at the time the bundle is made only:
nothing of them is needed to run an application.

  -I DIR      where raven_effect.glsl is (default: include/raven/shaders of this checkout)
  --animated  the window keeps drawing frames while the effect is shown (R_SHADER_ANIMATED)
  --backdrop  the effect reads the backdrop (R_SHADER_BACKDROP)
  --targets   which variants to make: spv,es30,msl (default: all)
"""
import argparse, importlib.util, os, struct, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))

def load_translator():
    """The translation of the Sokol backend (varying names for GL, descriptor numbers for Metal): gen_shaders.py."""
    path = os.path.join(ROOT, "src", "render", "sokol", "gen_shaders.py")
    spec = importlib.util.spec_from_file_location("gen_shaders", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

FLAG_BACKDROP, FLAG_ANIMATED = 1, 2

def build_bundle(flags, chunks):
    """chunks: [(tag, bytes)]. Header, table of 16 bytes per chunk, then the chunks aligned to 4 bytes."""
    table_end = 12 + 16 * len(chunks)
    out = b"RFX1" + struct.pack("<II", flags, len(chunks))
    offset = table_end
    body = b""
    for tag, data in chunks:
        assert len(tag) == 4
        out += tag + struct.pack("<III", offset, len(data), 0)
        padded = data + b"\0" * (-len(data) % 4)
        body += padded
        offset += len(padded)
    return out + body

def main():
    parser = argparse.ArgumentParser(description="Makes an effect bundle (.rfx)")
    parser.add_argument("input", help="the effect: GLSL (.frag) or SPIR-V (.spv)")
    parser.add_argument("-o", "--output", required=True)
    parser.add_argument("-I", dest="include", default=os.path.join(ROOT, "include", "raven", "shaders"))
    parser.add_argument("--animated", action="store_true")
    parser.add_argument("--backdrop", action="store_true")
    parser.add_argument("--targets", default="spv,es30,msl")
    args = parser.parse_args()
    targets = [t.strip().lower() for t in args.targets.split(",") if t.strip()]
    gs = load_translator()
    tmp = tempfile.mkdtemp()

    if args.input.endswith(".spv"):
        spv = args.input
    else:
        spv = os.path.join(tmp, "effect.spv")
        try:
            gs.run("glslangValidator", "-V", "-I" + args.include, args.input, "-o", spv)
        except subprocess.CalledProcessError as e:
            sys.exit("glslangValidator failed:\n" + (e.stdout or "") + (e.stderr or ""))
        except FileNotFoundError:
            sys.exit("glslangValidator is not installed (needed to compile GLSL; give a .spv to skip it)")
    data = open(spv, "rb").read()
    if len(data) < 20 or struct.unpack("<I", data[:4])[0] != 0x07230203:
        sys.exit("%s is not SPIR-V" % spv)

    chunks = []
    try:
        if "spv" in targets:
            chunks.append((b"SPV ", data))
        if "es30" in targets:
            es = os.path.join(tmp, "es.spv")
            open(es, "wb").write(data)
            # GLES 3.0 / WebGL 2 link varyings by name: the fragment inputs get the names of the vertex shader's outputs
            extra = []
            for location, name in enumerate(gs.VARYINGS):
                extra += ["--rename-interface-variable", "in", str(location), name]
            chunks.append((b"ES30", gs.cross(es, "es300", extra).encode()))
        if "msl" in targets:
            msl = os.path.join(tmp, "msl.spv")
            open(msl, "wb").write(gs.remap_descriptors(data))
            chunks.append((b"MSL ", gs.cross(msl, "msl").encode()))
    except FileNotFoundError:
        sys.exit("spirv-cross is not installed")
    except subprocess.CalledProcessError as e:
        sys.exit("spirv-cross failed:\n" + (e.stdout or "") + (e.stderr or ""))

    flags = (FLAG_BACKDROP if args.backdrop else 0) | (FLAG_ANIMATED if args.animated else 0)
    bundle = build_bundle(flags, chunks)
    open(args.output, "wb").write(bundle)
    print("%s: %d bytes (%s)" % (args.output, len(bundle), ", ".join(t.decode().strip() for t, _ in chunks)))

if __name__ == "__main__":
    main()
