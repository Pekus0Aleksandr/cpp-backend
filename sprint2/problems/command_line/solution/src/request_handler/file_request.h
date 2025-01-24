#pragma once
#include "base_request.h"
#include <filesystem>
#include "response.h"
#include "defs.h"

namespace http_handler {

	namespace fs = std::filesystem;
	
	class FileRequestHandler : public BaseRequestHandler {

	public:
		FileRequestHandler(const fs::path& static_path) : static_path_{ CheckStaticPath(static_path) } {}
		virtual ~FileRequestHandler() {}

	private:
		const fs::path static_path_;

	private: 
		TypeRequest ParseTarget(std::string_view target, std::string& res) const;
		FileRequestResult MakeGetResponse(const StringRequest& req, bool with_body) const override;
		FileRequestResult MakePostResponse(const StringRequest& req) const override;
		FileRequestResult MakeOptionsResponse(const StringRequest& req) const override;
		FileRequestResult MakePutResponse(const StringRequest& req) const override;
		FileRequestResult MakePatchResponse(const StringRequest& req) const override;
		FileRequestResult MakeDeleteResponse(const StringRequest& req) const override;
		static fs::path CheckStaticPath(const fs::path& path_static);
		FileRequestResult StaticFilesResponse(std::string_view responseText, bool with_body,
											  unsigned http_version, bool keep_alive) const;
		bool CheckFileExist(std::string& file) const;
		bool FileInRootStaticDir(std::string_view file) const;
	};

} //namespace http_handler
