#!/usr/bin/python
import sys
"""generate GL map tables from GLHCK tables"""

def find_gl_enum(enum):
    """maps GLHCK enum to GL enum"""

    map_to_zero = ['GLHCK_CLEAR_BIT_NONE', 'GLHCK_COMPARE_NONE']
    if enum in map_to_zero:
        return '0'

    ignore = ['GLHCK_FRAMEBUFFER_TYPE_LAST', 'GLHCK_TEXTURE_TYPE_LAST', 'GLHCK_HWBUFFER_TYPE_LAST']
    if enum in ignore:
        return 'ignore'

    files = ["/usr/include/GL/gl.h", "/usr/include/GL/glext.h"]

    for path in files:
        fle = open(path)
        if not fle:
            sys.exit("-!- failed to open gl.h include file")

        gle = enum
        if enum == 'GLHCK_COMPRESSED':
            gle = 'GL_UNSIGNED_BYTE'
        gle = gle.replace('COMPRESSED_RGBA_DXT5', 'COMPRESSED_RGBA_S3TC_DXT5_EXT')
        gle = gle.replace('COMPRESSED_RGB_DXT1', 'COMPRESSED_RGBA_S3TC_DXT1_EXT')
        gle = gle.replace('GLHCK_FRAMEBUFFER_READ', 'GL_READ_FRAMEBUFFER')
        gle = gle.replace('GLHCK_FRAMEBUFFER_DRAW', 'GL_DRAW_FRAMEBUFFER')
        gle = gle.replace('GLHCK_BUFFER', 'GL')
        gle = gle.replace('GLHCK', 'GL')
        while 1:
            lne = fle.readline()
            if not lne:
                break

            if lne[:7] != '#define':
                continue

            lne = lne[8:].split('\t')[0].split(' ')[0]
            if lne == gle:
                fle.close()
                return lne

        fle.close()

    return None

def generate():
    """generation function"""

    fle = open("include/glhck/glhck.h", "r")
    if not fle:
        sys.exit("-!- failed to open glhck.h include file")

    skip = ['glhckClearBufferBits']
    blocks = []
    mapped = []
    failures = []
    current_block = None
    for num, lne in enumerate(fle, 1):
        if current_block and lne[:1] == '}':
            blocks.append([current_block, mapped.copy()])
            current_block = None
            mapped = []

        if current_block:
            lne = lne[:-2].strip()
            lne = lne.split(' = ')[0]
            gle = find_gl_enum(lne)
            if gle:
                if gle != 'ignore':
                    mapped.append([lne, gle])
            else:
                failures.append([num, lne])

        if lne[:7] == 'PYGLMAP' and lne[13:-3] not in skip:
            current_block = lne[13:-3]

    if len(failures) == 0:
        for block, mapped in blocks:
            print("GLenum {}ToGL[] = {{".format(block))
            for glhe, gle in mapped:
                print("   {}, /* {} */".format(gle, glhe))
            print("};\n")

        for block, mapped in blocks:
            print("extern GLenum {}ToGL[];".format(block))

    for key, value in failures:
        print("MISSING {:d}: {}".format(key, value))

    fle.close()

generate()

# vim: set ts=8 sw=4 tw=0 :
