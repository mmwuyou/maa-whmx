/* Copyright 2024 周上行Ryer

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "Research.h"
#include "../Decode.h"
#include "../ReferenceDataSet.h"

#include <map>
#include <array>
#include <opencv2/imgproc.hpp>
#include <QtCore/QDebug>

namespace Rec::Research {

using namespace maa;

enum class GradeFaceCorner {
    Top,
    Left,
    Right,
};

struct GradeOptionPartInfo {
    int     index;
    MaaRect geo;
    cv::Mat image;
};

struct GradeOptionFaceInfo {
    int     index;
    MaaRect origin_geo;
    cv::Mat image;
};

static json::object make_ocr_params(const MaaRect &roi = MaaRect{0, 0, 0, 0}, const std::string &model = "ppocr_v4/zh_CN") {
    const json::object params{
        {"recognition", "OCR"                                           },
        {"model",       model                                           },
        {"roi",         json::array{roi.x, roi.y, roi.width, roi.height}},
    };
    return json::object{
        {"OCR", params}
    };
}

static void grade_face_tilt_correct(const cv::Mat &src, cv::Mat &dst, GradeFaceCorner corner) {
    switch (corner) {
        case GradeFaceCorner::Top: {
            cv::resize(src, dst, cv::Size(src.cols / 2, src.rows));
            const cv::Point2i center{dst.cols / 2, dst.rows / 2};
            const auto        transform = cv::getRotationMatrix2D(center, -45.0, 1.41421);
            cv::warpAffine(dst, dst, transform, dst.size());
        } break;
        case GradeFaceCorner::Left: {
            std::array<cv::Point2f, 3> src_points{
                cv::Point2f(0, 0),
                cv::Point2f(src.cols, src.rows / 3),
                cv::Point2f(0, src.rows / 3 * 2),
            };
            std::array<cv::Point2f, 3> dst_points{
                cv::Point2f(0, 0),
                cv::Point2f(src.cols, 0),
                cv::Point2f(0, src.rows / 3 * 2),
            };
            auto transform = cv::getAffineTransform(src_points.data(), dst_points.data());
            cv::warpAffine(src, dst, transform, src.size());
            dst = dst.rowRange(0, dst.rows / 3 * 2);
        } break;
        case GradeFaceCorner::Right: {
            std::array<cv::Point2f, 3> src_points{
                cv::Point2f(src.cols, 0),
                cv::Point2f(src.cols, src.rows / 3 * 2),
                cv::Point2f(0, src.rows / 3),
            };
            std::array<cv::Point2f, 3> dst_points{
                cv::Point2f(src.cols, 0),
                cv::Point2f(src.cols, src.rows / 3 * 2),
                cv::Point2f(0, 0),
            };
            auto transform = cv::getAffineTransform(src_points.data(), dst_points.data());
            cv::warpAffine(src, dst, transform, src.size());
            dst = dst.rowRange(0, dst.rows / 3 * 2);
        } break;
    }
}

cv::Mat crop_grade_option_face(const GradeOptionPartInfo &part, const GradeOptionFaceInfo &face) {
    const int dx = face.origin_geo.x - part.geo.x;
    const int dy = face.origin_geo.y - part.geo.y;
    const int w  = face.origin_geo.width;
    const int h  = face.origin_geo.height;
    return part.image.rowRange(dy, dy + h).colRange(dx, dx + w);
};

coro::Promise<AnalyzeResult> ParseGradeOptionsOnModify::research__parse_grade_options_on_modify(
    SyncContextHandle context, ImageHandle image, std::string_view task_name, std::string_view param) {
    //! FIXME: the method only works under 1280x720 resolution

    std::array<GradeOptionPartInfo, 2> parts;
    {
        const int width  = 372 - 168;
        const int height = 496 - 260;
        const int top    = 260;
        const int x_0    = 168;
        const int x_1    = 500;

        cv::Mat im(image->height(), image->width(), image->type(), image->raw_data());
        parts[0].index = 0;
        parts[0].geo   = MaaRect{x_0, top, width, height};
        parts[0].image = im.rowRange(top, top + height).colRange(x_0, x_0 + width);
        parts[1].index = 1;
        parts[1].geo   = MaaRect{x_1, top, width, height};
        parts[1].image = im.rowRange(top, top + height).colRange(x_1, x_1 + width);
    }

    std::array<GradeOptionFaceInfo, 6> faces;
    {
        int face_index = 0;
        for (const auto &part : parts) {
            {
                auto &face             = faces[face_index];
                face.index             = face_index++;
                face.origin_geo.x      = part.geo.x;
                face.origin_geo.y      = part.geo.y;
                face.origin_geo.width  = part.geo.width;
                face.origin_geo.height = part.geo.height / 2;
                const auto face_im     = crop_grade_option_face(part, face);
                grade_face_tilt_correct(face_im, face.image, GradeFaceCorner::Top);
            }
            {
                auto &face             = faces[face_index];
                face.index             = face_index++;
                face.origin_geo.x      = part.geo.x;
                face.origin_geo.y      = part.geo.y + part.geo.height / 4;
                face.origin_geo.width  = part.geo.width / 2;
                face.origin_geo.height = part.geo.y + part.geo.height - face.origin_geo.y;
                const auto face_im     = crop_grade_option_face(part, face);
                grade_face_tilt_correct(face_im, face.image, GradeFaceCorner::Left);
            }
            {
                auto &face             = faces[face_index];
                face.index             = face_index++;
                face.origin_geo.x      = part.geo.x + part.geo.width / 2;
                face.origin_geo.y      = part.geo.y + part.geo.height / 4;
                face.origin_geo.width  = part.geo.width / 2;
                face.origin_geo.height = part.geo.y + part.geo.height - face.origin_geo.y;
                const auto face_im     = crop_grade_option_face(part, face);
                grade_face_tilt_correct(face_im, face.image, GradeFaceCorner::Right);
            }
        }
    }

    struct RecTask {
        std::shared_ptr<details::Image> image;
        coro::Promise<AnalyzeResult>    promise;
    };

    const auto ocr_params = make_ocr_params();

    std::array<RecTask, 6> tasks;
    for (const auto &[index, geo, image] : faces) {
        auto &[image_object, promise] = tasks[index];
        image_object                  = details::Image::make();
        MaaSetImageRawData(image_object->handle(), image.data, image.cols, image.rows, image.type());
        promise = context->run_recognition(image_object, "OCR", ocr_params);
    }

    constexpr int UNKNOWN_GRADE = -1;

    static std::map<std::string_view /* utf8 */, int> GRADE_TABLE{
        {"\xe4\xb8\xad", 0},
        {"\xe8\x89\xaf", 1},
        {"\xe4\xbc\x98", 2},
    };

    json::array recog_results;
    for (int i = 0; i < tasks.size(); ++i) {
        int grade = UNKNOWN_GRADE;

        const auto recog_resp = co_await tasks[i].promise;
        const auto face_resp  = json::parse(recog_resp.rec_detail);
        do {
            //! TODO: fallback to "all" if "best" is not found
            if (!face_resp->contains("best")) { break; }
            const auto &best_recog = face_resp->at("best");
            if (!best_recog.contains("text")) { break; }
            const auto &text = best_recog.at("text").as_string();
            if (GRADE_TABLE.count(text)) { grade = GRADE_TABLE[text]; }
        } while (0);

        const auto &geo = faces[i].origin_geo;
        recog_results.push_back(json::object({
            {"index", i                                               },
            {"grade", grade                                           },
            {"box",   json::array{geo.x, geo.y, geo.width, geo.height}},
        }));
    }

    AnalyzeResult resp;
    resp.rec_box    = MaaRect{0, 0, 0, 0};
    resp.rec_detail = recog_results.to_string();
    resp.result     = true;

    co_return resp;
}

bool ParseAnecdote::parse_params(ParseAnecdoteParam &param_out, MaaStringView raw_param) {
    auto opt_params = json::parse(raw_param);
    if (!opt_params.has_value()) { return false; }
    if (!opt_params->is_object()) { return false; }

    const auto &params = opt_params.value().as_object();

    if (params.contains("category")) {
        param_out.category = params.at("category").as_string();
    } else {
        return false;
    }

    param_out.enable_fuzzy_search = params.get("enable_fuzzy_search", false);
    param_out.start_stage         = params.get("start_stage", -1);

    return true;
}

coro::Promise<AnalyzeResult> ParseAnecdote::research__parse_anecdote(
    SyncContextHandle context, ImageHandle image, std::string_view task_name, std::string_view param) {
    //! FIXME: the method only works under 1280x720 resolution

    AnalyzeResult resp;
    resp.result = false;

    ParseAnecdoteParam opt;
    if (!parse_params(opt, param.data())) {
        qDebug("%s: invalid arguments", task_name.data());
        co_return resp;
    }

    //! TODO: support fuzzy search
    const auto opt_category = Ref::ResearchAnecdoteSet::instance()->entry(opt.category);
    if (!opt_category.has_value()) { co_return resp; }
    const auto &category = opt_category.value().get();

    const MaaRect roi_title{600, 120, 460, 32};
    const MaaRect roi_content{600, 170, 490, 196};

    const int opt_size_w  = 350;
    const int opt_size_h  = 28;
    const int opt_x       = 620;
    const int first_opt_y = 400;
    const int opt_dy      = 68;

    const auto ocr_params = make_ocr_params();

    const auto title_resp = co_await context->run_recognition(image, "OCR", make_ocr_params(roi_title));
    const auto opt_title  = parse_and_get_best_ocr_record(json::parse(title_resp.rec_detail).value());
    if (!opt_title.has_value()) { co_return resp; }

    const auto title = opt_title.value();

    const auto content_resp = co_await context->run_recognition(image, "OCR", make_ocr_params(roi_content));
    const auto opt_content  = parse_and_get_full_text_ocr_result(json::parse(content_resp.rec_detail).value());
    if (!opt_content.has_value()) { co_return resp; }

    const auto content = opt_content.value();

    const auto opt_entry = category.entry(title.text);
    if (!opt_entry.has_value()) { co_return resp; }
    const auto &entry = opt_entry.value().get();

    //! TODO: check option stage
    //! TODO: recognize option stage, i.e. recog content or text of options

    if (opt.start_stage == -1) { Q_UNIMPLEMENTED(); }

    json::object resp_data{
        {"category", opt.category   },
        {"name",     entry.name()   },
        {"stage",    opt.start_stage},
    };

    resp.result     = true;
    resp.rec_detail = resp_data.to_string();
    co_return resp;
}

} // namespace Rec::Research
