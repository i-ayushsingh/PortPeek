#include "qrcode.h"
#include "qrcodegen.h"
#include <vector>

namespace QrCode {

QrMatrix Generate(const std::string& text) {
    QrMatrix result;
    if (text.empty()) return result;

    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

    bool ok = qrcodegen_encodeText(
        text.c_str(),
        tempBuffer,
        qrcode,
        qrcodegen_Ecc_MEDIUM,
        qrcodegen_VERSION_MIN,
        qrcodegen_VERSION_MAX,
        qrcodegen_Mask_AUTO,
        true
    );

    if (!ok) return result;

    result.size = qrcodegen_getSize(qrcode);
    result.modules.assign(result.size * result.size, 0);

    for (int y = 0; y < result.size; ++y) {
        for (int x = 0; x < result.size; ++x) {
            result.Set(x, y, qrcodegen_getModule(qrcode, x, y));
        }
    }

    return result;
}

} // namespace QrCode
