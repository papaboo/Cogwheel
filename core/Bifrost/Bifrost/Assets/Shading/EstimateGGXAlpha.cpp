// Estimate the alpha of the GGX distribution that with the given maximal PDF from a angle.
// ------------------------------------------------------------------------------------------------
// Copyright (C) 2018, Bifrost. See AUTHORS.txt for authors
//
// This program is open source and distributed under the New BSD License.
// See LICENSE.txt for more detail.
// ------------------------------------------------------------------------------------------------
// Generated by AlbedoComputation application.
// ------------------------------------------------------------------------------------------------

#include <Bifrost/Assets/Shading/Fittings.h>
#include <Bifrost/Math/Utils.h>

namespace Bifrost {
namespace Assets {
namespace Shading {
namespace EstimateGGXAlpha {

const int alpha_sample_count = 1024;
const int cos_theta_sample_count = 32;
const int max_PDF_sample_count = 32;

const float alphas[] = {
    // cos(theta) 0
    1, 1, 1, 1, 1, 0.938842, 0.872006, 0.817135, 0.77055, 0.729984, 0.693946, 0.6614, 0.631598, 0.603985, 0.578125, 0.553678, 0.530359, 0.507931, 0.486183, 0.464928, 0.443978, 0.423155, 0.402264, 0.381086, 0.35936, 0.336744, 0.312771, 0.286718, 0.257359, 0.222206, 0.174377, 0.00390625, 
    // cos(theta) 0.0322581
    1, 1, 1, 1, 1, 0.928219, 0.861368, 0.806482, 0.759885, 0.719309, 0.68326, 0.650704, 0.620894, 0.593271, 0.567404, 0.542948, 0.519621, 0.497185, 0.47543, 0.454164, 0.433207, 0.412373, 0.391472, 0.370283, 0.348545, 0.325916, 0.301928, 0.275856, 0.246471, 0.211281, 0.163391, 0.00195313, 
    // cos(theta) 0.0645161
    1, 1, 1, 1, 1, 0.917741, 0.850842, 0.795916, 0.749283, 0.708675, 0.672597, 0.640014, 0.610178, 0.582529, 0.556638, 0.532158, 0.508808, 0.486348, 0.464569, 0.443281, 0.422298, 0.40144, 0.380512, 0.359297, 0.337528, 0.314867, 0.29084, 0.264725, 0.235291, 0.200035, 0.152069, 0.000976563, 
    // cos(theta) 0.0967742
    1, 1, 1, 1, 0.992891, 0.907417, 0.84044, 0.785448, 0.738758, 0.698098, 0.661973, 0.629346, 0.599469, 0.571783, 0.545854, 0.521338, 0.497952, 0.475458, 0.453646, 0.432322, 0.411306, 0.390413, 0.36945, 0.348198, 0.326391, 0.303692, 0.279627, 0.253475, 0.224007, 0.188747, 0.140915, 0.000976563, 
    // cos(theta) 0.129032
    1, 1, 1, 1, 0.982832, 0.897225, 0.830142, 0.775061, 0.728292, 0.687564, 0.651377, 0.618692, 0.588762, 0.561027, 0.53505, 0.51049, 0.487062, 0.464526, 0.442673, 0.421311, 0.400257, 0.37933, 0.358332, 0.337048, 0.315212, 0.292492, 0.268412, 0.242264, 0.212843, 0.177734, 0.130455, 0.000976563, 
    // cos(theta) 0.16129
    1, 1, 1, 1, 0.972921, 0.887148, 0.819931, 0.764738, 0.717875, 0.677062, 0.6408, 0.608047, 0.578056, 0.550262, 0.524235, 0.499624, 0.476151, 0.453573, 0.431681, 0.410284, 0.389198, 0.368244, 0.347225, 0.325928, 0.30409, 0.281384, 0.257345, 0.231285, 0.202038, 0.167309, 0.121056, 0.000976563, 
    // cos(theta) 0.193548
    1, 1, 1, 1, 0.96314, 0.877165, 0.80979, 0.754467, 0.707493, 0.666584, 0.630238, 0.597412, 0.567352, 0.539499, 0.513417, 0.48876, 0.465244, 0.44263, 0.420708, 0.399286, 0.378185, 0.357224, 0.33621, 0.314934, 0.293139, 0.270504, 0.246586, 0.22072, 0.191807, 0.157703, 0.112854, 0.000488281, 
    // cos(theta) 0.225806
    1, 1, 1, 1, 0.95347, 0.867264, 0.799707, 0.744236, 0.69714, 0.656127, 0.619691, 0.586786, 0.556662, 0.52875, 0.502619, 0.477921, 0.454372, 0.431736, 0.4098, 0.388376, 0.367285, 0.346348, 0.325377, 0.304169, 0.282473, 0.259983, 0.236277, 0.210728, 0.182304, 0.149021, 0.105789, 0.000488281, 
    // cos(theta) 0.258065
    1, 1, 1, 1, 0.943896, 0.857428, 0.78967, 0.734039, 0.68681, 0.645689, 0.609162, 0.576183, 0.545996, 0.518034, 0.491865, 0.46714, 0.443578, 0.420938, 0.399014, 0.377615, 0.356567, 0.335694, 0.314816, 0.293728, 0.2722, 0.249933, 0.22653, 0.201405, 0.173599, 0.14127, 0.0997162, 0.000488281, 
    // cos(theta) 0.290323
    1, 1, 1, 1, 0.934402, 0.847645, 0.77967, 0.723867, 0.676504, 0.635274, 0.59866, 0.565613, 0.535374, 0.507378, 0.481187, 0.456455, 0.432901, 0.410286, 0.388403, 0.367067, 0.346104, 0.325342, 0.304605, 0.283701, 0.262403, 0.240433, 0.217415, 0.192801, 0.165691, 0.134377, 0.0944748, 0.000488281, 
    // cos(theta) 0.322581
    1, 1, 1, 1, 0.924973, 0.837904, 0.769699, 0.713722, 0.666224, 0.624889, 0.588198, 0.555096, 0.52482, 0.496808, 0.470619, 0.445908, 0.422391, 0.399834, 0.378029, 0.356794, 0.335958, 0.315355, 0.294813, 0.274145, 0.253139, 0.231529, 0.208958, 0.184912, 0.158548, 0.128258, 0.0899124, 0.000488281, 
    // cos(theta) 0.354839
    1, 1, 1, 1, 0.915598, 0.828197, 0.759753, 0.7036, 0.655972, 0.614544, 0.57779, 0.544651, 0.514362, 0.486359, 0.460198, 0.435537, 0.412092, 0.389628, 0.367941, 0.346849, 0.326187, 0.305787, 0.285486, 0.265106, 0.244438, 0.223232, 0.201151, 0.177706, 0.1521, 0.122807, 0.0859184, 0.000488281, 
    // cos(theta) 0.387097
    1, 1, 1, 1, 0.906265, 0.818518, 0.74983, 0.693505, 0.645758, 0.604254, 0.567457, 0.534304, 0.504029, 0.476063, 0.449964, 0.425386, 0.402048, 0.379715, 0.358185, 0.337276, 0.316826, 0.296671, 0.276653, 0.256598, 0.236305, 0.215536, 0.193966, 0.171131, 0.146271, 0.117935, 0.0823898, 0.000488281, 
    // cos(theta) 0.419355
    1, 1, 1, 1, 0.896965, 0.808859, 0.739929, 0.683444, 0.635595, 0.594035, 0.55722, 0.524081, 0.493851, 0.465954, 0.439949, 0.415491, 0.392298, 0.370132, 0.348795, 0.328106, 0.307902, 0.288028, 0.268324, 0.248621, 0.228727, 0.20841, 0.187361, 0.165131, 0.140993, 0.113556, 0.0792427, 0.000488281, 
    // cos(theta) 0.451613
    1, 1, 1, 1, 0.88769, 0.799217, 0.730055, 0.673424, 0.625495, 0.583905, 0.547102, 0.514009, 0.483855, 0.456062, 0.430189, 0.405885, 0.382869, 0.360905, 0.339793, 0.319355, 0.299429, 0.27986, 0.26049, 0.241159, 0.221678, 0.201818, 0.181284, 0.159643, 0.136196, 0.109604, 0.0764236, 0.000488281, 
    // cos(theta) 0.483871
    1, 1, 1, 1, 0.87843, 0.789593, 0.720209, 0.663456, 0.615475, 0.573886, 0.537127, 0.504115, 0.474072, 0.446419, 0.420709, 0.396592, 0.373785, 0.352054, 0.331195, 0.311033, 0.291405, 0.27216, 0.253141, 0.23419, 0.215122, 0.195719, 0.17569, 0.154615, 0.131823, 0.106018, 0.0738754, 0.000488281, 
    // cos(theta) 0.516129
    1, 1, 1, 0.991797, 0.869183, 0.779986, 0.710402, 0.653553, 0.60555, 0.563997, 0.527318, 0.494423, 0.464525, 0.437044, 0.41153, 0.387631, 0.365063, 0.343588, 0.323004, 0.303137, 0.283823, 0.264913, 0.246252, 0.227684, 0.209028, 0.190069, 0.170527, 0.149994, 0.127819, 0.102749, 0.0715637, 0.000488281, 
    // cos(theta) 0.548387
    1, 1, 1, 0.983192, 0.859941, 0.7704, 0.700642, 0.643728, 0.595741, 0.55426, 0.517698, 0.484954, 0.455236, 0.427959, 0.402671, 0.379015, 0.356707, 0.335508, 0.315217, 0.295657, 0.276668, 0.258097, 0.239796, 0.221607, 0.203354, 0.184828, 0.165752, 0.145737, 0.124142, 0.0997524, 0.0694504, 0.000488281, 
    // cos(theta) 0.580645
    1, 1, 1, 0.9746, 0.850706, 0.760839, 0.690937, 0.633999, 0.586065, 0.544696, 0.508286, 0.475728, 0.446222, 0.419178, 0.394139, 0.370749, 0.34872, 0.327812, 0.307825, 0.28858, 0.269918, 0.251689, 0.233744, 0.215927, 0.198067, 0.179956, 0.161329, 0.1418, 0.120749, 0.0969963, 0.0675125, 0.000488281, 
    // cos(theta) 0.612903
    1, 1, 1, 0.966009, 0.841475, 0.751308, 0.681303, 0.62438, 0.576541, 0.535321, 0.499102, 0.466761, 0.437496, 0.410707, 0.385941, 0.362833, 0.341095, 0.32049, 0.300812, 0.281887, 0.263554, 0.245663, 0.228066, 0.210613, 0.193131, 0.175421, 0.157217, 0.138149, 0.117611, 0.0944519, 0.0657272, 0.000488281, 
    // cos(theta) 0.645161
    1, 1, 1, 0.957415, 0.832251, 0.741819, 0.671751, 0.614887, 0.567185, 0.526154, 0.490156, 0.458063, 0.429063, 0.402554, 0.378073, 0.355261, 0.333826, 0.313528, 0.294164, 0.275557, 0.257549, 0.23999, 0.222734, 0.205633, 0.188517, 0.171186, 0.153387, 0.134754, 0.114697, 0.0920944, 0.0640755, 0.000488281, 
    // cos(theta) 0.677419
    1, 1, 1, 0.948811, 0.823032, 0.73238, 0.662296, 0.605537, 0.558014, 0.517206, 0.481462, 0.449642, 0.420929, 0.394714, 0.370534, 0.348026, 0.326899, 0.306911, 0.287861, 0.269569, 0.251881, 0.234647, 0.217722, 0.200958, 0.184192, 0.167226, 0.149811, 0.131588, 0.111982, 0.089901, 0.0625381, 0.000244141, 
    // cos(theta) 0.709677
    1, 1, 1, 0.940189, 0.813828, 0.723001, 0.652953, 0.596346, 0.54904, 0.50849, 0.473028, 0.441504, 0.413094, 0.387186, 0.363316, 0.341118, 0.320301, 0.300623, 0.281883, 0.263903, 0.246527, 0.229609, 0.213003, 0.196566, 0.180132, 0.163513, 0.146461, 0.128628, 0.109447, 0.0878563, 0.0611076, 0.000244141, 
    // cos(theta) 0.741935
    1, 1, 1, 0.931551, 0.804642, 0.713695, 0.643734, 0.587324, 0.540275, 0.500013, 0.464857, 0.433647, 0.405554, 0.379963, 0.356409, 0.334524, 0.314016, 0.294646, 0.276211, 0.258536, 0.241465, 0.224852, 0.208554, 0.192429, 0.176315, 0.160027, 0.14332, 0.125853, 0.107075, 0.0859413, 0.0597725, 0.000244141, 
    // cos(theta) 0.774194
    1, 1, 1, 0.922892, 0.795481, 0.704473, 0.634654, 0.578485, 0.531727, 0.491783, 0.456953, 0.426072, 0.398306, 0.373037, 0.349801, 0.328229, 0.308029, 0.288962, 0.270826, 0.253448, 0.236672, 0.220354, 0.204353, 0.188526, 0.172718, 0.156744, 0.140364, 0.123245, 0.104847, 0.0841446, 0.0585175, 0.000244141, 
    // cos(theta) 0.806452
    1, 1, 1, 0.914209, 0.786356, 0.695348, 0.625724, 0.569839, 0.523405, 0.483801, 0.449314, 0.418773, 0.39134, 0.3664, 0.343481, 0.32222, 0.302323, 0.283554, 0.265711, 0.248621, 0.23213, 0.216096, 0.200379, 0.18484, 0.169323, 0.153648, 0.137579, 0.12079, 0.102749, 0.0824566, 0.0573387, 0.000244141, 
    // cos(theta) 0.83871
    1, 1, 1, 0.905508, 0.777273, 0.686333, 0.616956, 0.561394, 0.515312, 0.476068, 0.441937, 0.411745, 0.38465, 0.360036, 0.337435, 0.316481, 0.296883, 0.278404, 0.260846, 0.244036, 0.22782, 0.212059, 0.196616, 0.181352, 0.166113, 0.150723, 0.13495, 0.118473, 0.100771, 0.080864, 0.0562286, 0.000244141, 
    // cos(theta) 0.870968
    1, 1, 1, 0.896786, 0.768247, 0.677437, 0.608358, 0.553156, 0.507452, 0.468583, 0.434819, 0.40498, 0.378225, 0.353937, 0.331649, 0.310997, 0.291692, 0.273497, 0.256214, 0.239675, 0.223726, 0.208228, 0.193047, 0.178045, 0.163073, 0.147953, 0.132462, 0.116282, 0.0989017, 0.079361, 0.0551796, 0.000244141, 
    // cos(theta) 0.903226
    1, 1, 1, 0.88805, 0.759285, 0.668674, 0.599941, 0.54513, 0.499824, 0.461344, 0.427953, 0.39847, 0.372054, 0.34809, 0.326111, 0.305756, 0.286736, 0.268817, 0.251802, 0.235523, 0.219831, 0.204586, 0.189656, 0.174906, 0.160187, 0.145327, 0.130103, 0.114204, 0.0971317, 0.0779362, 0.0541878, 0.000244141, 
    // cos(theta) 0.935484
    1, 1, 1, 0.879305, 0.750401, 0.660055, 0.591709, 0.537317, 0.492426, 0.454345, 0.421331, 0.392205, 0.366127, 0.342483, 0.320807, 0.300742, 0.282, 0.264349, 0.247593, 0.231567, 0.216122, 0.201119, 0.186431, 0.171921, 0.157445, 0.142832, 0.127865, 0.112234, 0.0954533, 0.0765858, 0.0532455, 0.000244141, 
    // cos(theta) 0.967742
    1, 1, 1, 0.870558, 0.741602, 0.651586, 0.58367, 0.52972, 0.485258, 0.447581, 0.414948, 0.386178, 0.360434, 0.337103, 0.315725, 0.295943, 0.277472, 0.260079, 0.243575, 0.227792, 0.212584, 0.197816, 0.183359, 0.16908, 0.154836, 0.140459, 0.125734, 0.110361, 0.0938568, 0.0753021, 0.0523529, 0.000244141, 
    // cos(theta) 1
    1, 1, 1, 0.861814, 0.732904, 0.643275, 0.575824, 0.522338, 0.478315, 0.441048, 0.408794, 0.380377, 0.354961, 0.33194, 0.310853, 0.291347, 0.273137, 0.255997, 0.239735, 0.224186, 0.209208, 0.194664, 0.180429, 0.16637, 0.152349, 0.138197, 0.123707, 0.108578, 0.0923367, 0.0740814, 0.0515022, 0.000244141, 
};

float encode_PDF(float p) { return p / (1.0f + p); }
float decode_PDF(float e) { return e / (1.0f - e); }

float estimate_alpha(float cos_theta, float max_PDF) {
    using namespace Bifrost::Math;

    float encoded_PDF = encode_PDF(max_PDF);

    int lower_cos_theta_row = int(cos_theta * (cos_theta_sample_count - 1));
    int upper_cos_theta_row = min(lower_cos_theta_row + 1, cos_theta_sample_count - 1);

    int lower_encoded_PDF_column = int(encoded_PDF * (max_PDF_sample_count - 1));
    int upper_encoded_PDF_column = min(lower_encoded_PDF_column + 1, max_PDF_sample_count - 1);
    float encoded_PDF_t = encoded_PDF * (max_PDF_sample_count - 1) - lower_encoded_PDF_column;

    // Interpolate by encoded PDF
    const float* lower_alpha_row = alphas + lower_cos_theta_row * cos_theta_sample_count;
    float lower_alpha = lerp(lower_alpha_row[lower_encoded_PDF_column], lower_alpha_row[upper_encoded_PDF_column], encoded_PDF_t);

    const float* upper_alpha_row = alphas + upper_cos_theta_row * cos_theta_sample_count;
    float upper_alpha = lerp(upper_alpha_row[lower_encoded_PDF_column], upper_alpha_row[upper_encoded_PDF_column], encoded_PDF_t);

    // Interpolate by cos_theta
    float cos_theta_t = cos_theta * (cos_theta_sample_count - 1) - lower_cos_theta_row;
    return lerp(lower_alpha, upper_alpha, cos_theta_t);
}

} // NS EstimateGGXAlpha
} // NS Shading
} // NS Assets
} // NS Bifrost
