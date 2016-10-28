#include "./util.hpp"

using namespace cc;

namespace wav {
typedef struct {
    char           riff_id[4];   // "riff"
    unsigned int   size;         // filesize - 8
    char           wav_id[4];    // "WAVE"
    char           fmt_id[4];    // "fmt "
    unsigned int   fmt_size;     // fmtチャンクのバイト数
    unsigned short format;       // フォーマット
    unsigned short channels;     // チャンネル数
    unsigned int   sample_rate;  // サンプリングレート
    unsigned int   byte_per_sec; // データ速度
    unsigned short block_size;   // ブロックサイズ
    unsigned short bit;          // 量子化ビット数
    char           data_id[4];   // "data"
    unsigned int   data_size;    // 波形データのバイト数
} header;

/**
 * ref : https://gist.github.com/yomakkkk/2290842
 */
void read(std::string fn, vec_t &data) {
    wav::header   header;
    std::ifstream ifs(fn, std::ios::in | std::ios::binary);
    if (!ifs) {
        throw std::runtime_error(format_str("failed to open %s", fn.c_str()));
    }

    ifs.read((char *)&header, sizeof(header));

    while (!ifs.eof()) {
        short buf;
        ifs.read((char *)&buf, sizeof(buf));
        data.push_back((float)buf / pow(2.0f, header.bit - 1)); // normalize to -1.0 ~ 1.0
    }
    ifs.close();
}

// void write(std::string fn, std::vector<short> &data, header &header) {
//     std::ofstream ofs(fn, std::ios::out | std::ios::binary | std::ios::trunc);
//     if (!ofs) {
//         throw std::runtime_error(format_str("failed to write %s", fn));
//     }
//
//     header.data_size = data.size();
//     ofs.write((char *)&header, sizeof(header));
//
//     auto it = data.begin();
//     while (it != data.end()) {
//         short buf = *(it++);
//         ofs.write((char *)&buf, sizeof(buf));
//     }
//     ofs.close();
// }

void pre_emphasis(vec_t &r) {
    vec_t tmp(r.size());

    tmp[0] = r[0];

    // ignore first element
    for (size_t i = 1; i < r.size(); i++) {
        tmp[i] = r[i] - 0.97 * r[i - 1];
    }

    std::copy(tmp.begin(), tmp.end(), r.begin());
}

void window_hanning(vec_t &r) {
    size_t N = r.size();

    // apply hanning window
    for (size_t i = 0; i < N; i++) {
        r[i] *= (0.5 - 0.5 * cos(2 * M_PI * i / (N - 1)));
    }
}

void fourier(const vec_t &raw, vec_t &re, vec_t &im) {
    int N    = raw.size();
    int FREQ = re.size();

    std::fill(re.begin(), re.end(), 0.0f);
    std::fill(im.begin(), im.end(), 0.0f);

    // apply fourier transform
    for (int i = 0; i < FREQ; i++) {
        for (int k = 0; k < N; k++) {
            re[i] += (float)raw[k]  * cos(2.0f * M_PI * k * i / FREQ);
            im[i] += (float)-raw[k] * sin(2.0f * M_PI * k * i / FREQ);
        }
    }
}

void amplitude(const vec_t &re, const vec_t &im, vec_t &amp) {
    int N = re.size();

    for (int i = 0; i < N; i++) {
        amp[i] = sqrt(re[i] * re[i] + im[i] * im[i]);
    }
}

float hz2mel(float f) {
    return 1127.01048 * std::log(f / 700.0 + 1.0);
}

float mel2hz(float m) {
    return 700.0 * (std::exp(m / 1127.01048) - 1.0);
}

void melfilter(vec_t &amp, vec_t &mel_x, vec_t &mel_y) {
    int NYQ     = amp.size();
    int channel = 20;

    int   melmax = hz2mel(NYQ);
    float df     = 1;
    float dmel   =  melmax / (channel + 1);

    vec_t            m_centers(channel);
    vec_t            f_centers(channel);
    std::vector<int> i_centers(channel);
    for (int i = 0; i < channel; i++) {
        m_centers[i] = (i + 1) * dmel;
        f_centers[i] = mel2hz(m_centers[i]);
        i_centers[i] = (int)(f_centers[i] / df);
    }
    mel_x = f_centers;

    std::vector<int> i_s(channel);
    std::vector<int> i_e(channel);
    for (int i = 0; i < channel; i++) {
        i_s[i] = (i - 1) == -1      ? 0   : i_centers[i - 1];
        i_e[i] = (i + 1) == channel ? NYQ : i_centers[i + 1];
    }

    std::vector<vec_t> bank;
    for (int c = 0; c < channel; c++) {
        vec_t filter(NYQ);

        for (int i = i_s[c]; i < i_centers[c]; i++) {
            filter[i] = (1.0f / (i_centers[c] - i_s[c])) * (i - i_s[c]);
        }

        for (int i = i_centers[c]; i < i_e[c]; i++) {
            filter[i] = 1.0 - (1.0f / (i_e[c] - i_centers[c])) * (i - i_centers[c]);
        }
        bank.push_back(filter);
    }

    for (int c = 0; c < channel; c++) {
        float sum = 0.0f;
        for (int i = 0; i < NYQ; i++) {
            sum += amp[i] * bank[c][i];
        }
        mel_y[c] = sum;
    }
}

/**
 * ref : http://tony-mooori.blogspot.jp/2016/02/dctpythonpython.html
 */
void dct(vec_t &s, vec_t &d) {
    int N = s.size();

    std::fill(d.begin(), d.end(), 0.0f);

    for (int k = 0; k < N; k++) {
        for (int i = 0; i < N; i++) {
            if (k == 0) {
                d[k] += s[i] * sqrt(1.0f / N);
            } else {
                d[k] += s[i] * sqrt(2.0f / N) * cos((2 * i + 1) * k * M_PI / 2.0f / N);
            }
        }
    }
}

void idct(vec_t &s, vec_t &d) {
    int N = s.size();

    std::fill(d.begin(), d.end(), 0.0f);

    for (int k = 0; k < N; k++) {
        for (int i = 0; i < N; i++) {
            if (k == 0) {
                d[i] += s[k] * sqrt(1.0f / N);
            } else {
                d[i] += s[k] * sqrt(2.0f / N) * cos((2 * i + 1) * k * M_PI / 2.0f / N);
            }
        }
    }
}

void log_spectrum(vec_t &a) {
    int N = a.size();

    for (int i = 0; i < N; i++) {
        a[i] = 20.0f * log10f(a[i]);
    }
}
} // namespace wav

int main(int argc, char *argv[]) {
    try {
        vec_t raw;

        // 音声データを読み込む
        wav::read("a.wav", raw);

        // リサイズする
        const int N = 1024;
        raw.resize(N);

        // プリエンファシスをかけて高音を強調する
        wav::pre_emphasis(raw);

        // ハニング窓関数をかける
        wav::window_hanning(raw);

        // フーリエ変換する
        const int FRQ = 44000;
        vec_t     re(FRQ);
        vec_t     im(FRQ);
        wav::fourier(raw, re, im);

        // 振幅スペクトルにする
        vec_t amp(FRQ);
        wav::amplitude(re, im, amp);

        // ナイキスト周波数でカットする
        const int NYQ = FRQ / 2;
        amp.resize(NYQ);

        // メルフィルタバンクと内積をとって次元を減らす
        const int DIM = 20;
        vec_t     mel_x(DIM);
        vec_t     mel_y(DIM);
        wav::melfilter(amp, mel_x, mel_y);

        // 対数スペクトルに変換する
        wav::log_spectrum(mel_y);

        // 離散コサイン変換 (DCT-II) でケプストラム領域に移す
        vec_t cepstrum(DIM);
        wav::dct(mel_y, cepstrum);

        // リフタリングで定常成分と高次成分を除去してMFCCを得る
        const int MFCC_DIM = 12;
        vec_t     mfcc(MFCC_DIM);
        std::copy(cepstrum.begin() + 1, cepstrum.begin() + MFCC_DIM + 1, mfcc.begin());

        for (auto f : mfcc) {
            std::cout << f << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << colorant('y', format_str("error: %s", e.what())) << std::endl;
    }
}

