#include <opencv2/opencv.hpp>

#include "../detail/types.h"

namespace cv
{

inline trace_x::image_stream &operator<<(trace_x::image_stream &os, const Mat &cv_mat)
{
    if(!cv_mat.empty())
    {
        int8_t unit_type;

        switch(cv_mat.depth())
        {
        case CV_8U:  unit_type = trace_x::T_8U;  break;
        case CV_8S:  unit_type = trace_x::T_8S;  break;
        case CV_16U: unit_type = trace_x::T_16U; break;
        case CV_16S: unit_type = trace_x::T_16S; break;
        case CV_32S: unit_type = trace_x::T_32S; break;
        case CV_32F: unit_type = trace_x::T_32F; break;
        case CV_64F: unit_type = trace_x::T_64F; break;
        default: break;
        }

        os.data.assign_numeric_matrix(cv_mat.cols, cv_mat.rows, cv_mat.channels(), unit_type, cv_mat.data, cv_mat.step[0]);

        switch(cv_mat.channels())
        {
        case 1: os.data.set_band_map(trace_x::make_vector<uint8_t>() << trace_x::BAND_GRAY); break;
        case 2: os.data.set_band_map(trace_x::make_vector<uint8_t>() << trace_x::BAND_GRAY << trace_x::BAND_ALPHA); break;
        case 3: os.data.set_band_map(trace_x::make_vector<uint8_t>() << trace_x::BAND_BLUE << trace_x::BAND_GREEN << trace_x::BAND_RED); break;
        case 4: os.data.set_band_map(trace_x::make_vector<uint8_t>() << trace_x::BAND_BLUE << trace_x::BAND_GREEN << trace_x::BAND_RED << trace_x::BAND_ALPHA); break;
        default: break;
        }
    }

    return os;
}

}
