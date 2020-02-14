import os
import argparse

def get_file_as_string(filepath):
    with open(filepath, "r") as file:
        return file.read()

def get_dir_as_strings(directory):
    file_strings = []
    for file in os.listdir(directory):
        filepath = os.path.join(directory, file)
        if os.path.isdir(filepath):
            subdir_strings = get_dir_as_strings(filepath)
            file_strings.extend(subdir_strings)
        elif os.path.isfile(filepath):
            string = get_file_as_string(filepath)
            file_strings.append((filepath, string))
    
    return file_strings

def generate_header_file(file_strings):
    return """
#ifndef __WEBSERVER_H__ 
#define __WEBSERVER_H__

#include <esp_http_server.h>

httpd_handle_t start_webserver(uint32_t port);

#endif
    """

def generate_source_file(file_strings, header_filename):
    template = """
#include "{header_filename}"
#include <esp_log.h>
#include <esp_http_server.h>
#include <stdlib.h>

#define TAG "webserver"

// files    
{files}

// functions
{functions}

// uris
{uris}

httpd_handle_t start_webserver(uint32_t port) {{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.max_uri_handlers = {max_handles};

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {{
{code}
        return server;
    }}

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}}
    """

    function_template = """
esp_err_t {function}(httpd_req_t *request) {{
    return httpd_resp_send(request, {file}, sizeof({file}));
}}
    """

    code_template = "        httpd_register_uri_handler(server, &{uri});"

    uri_template = """
httpd_uri_t {name} = {{  
    .uri = "{filename}",
    .method = HTTP_GET,
    .handler = {function}
}};
    """

    char_array_template = "const char {name}[] = \"{string}\";"

    file_arrays = []
    functions = []
    uris = []
    code_lines = []

    for i, (filename, file) in enumerate(file_strings):
        file_name = f"FILEDATA_{i}"
        function_name = f"GET_FILE_{i}"
        uri_name = f"URI_FILE_{i}"

        file_array = char_array_template.format(name=escape_source(file_name), string=escape_source(file))
        function = function_template.format(function=function_name, file=file_name)
        uri = uri_template.format(name=uri_name, filename=escape_source(filename), function=function_name)
        code_line = code_template.format(uri=uri_name)

        file_arrays.append(file_array)
        functions.append(function)
        uris.append(uri)
        code_lines.append(code_line)

        if filename == "/index.html":
            uri = uri_template.format(name="ROOT_URI", filename="/", function=function_name)
            code_line = code_template.format(uri="ROOT_URI")
            uris.append(uri)
            code_lines.append(code_line)

    files = "\n".join(file_arrays)
    functions_str = "\n".join(functions)
    uris_str = "\n".join(uris)
    code = "\n".join(code_lines)

    source_file = template.format(
        header_filename=header_filename, 
        files=files,
        functions=functions_str,
        uris=uris_str,
        code=code, 
        max_handles=len(uris))
    return source_file

def escape_source(string):
    escaped = []
    for char in string:
        if char in ('"', '\\'):
            escaped.append(f"\\{char}")
        elif char in ('\n'):
            escaped.append("\\n")
        else:
            escaped.append(char)
    return ''.join(escaped)

def reroot_file_strings(file_strings, root):
    for filepath, file in file_strings:
        filepath = os.path.relpath(filepath, root)
        filepath = filepath.replace("\\", "/")
        filepath = f"/{filepath}"
        yield (filepath, file)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input_dir", type=str)
    parser.add_argument("--output_dir", default="components/web_server/include/web_server/", type=str)
    parser.add_argument("--output_file", default="server.{ext}" ,type=str)

    args = parser.parse_args()

    file_strings = get_dir_as_strings(args.input_dir)
    file_strings = reroot_file_strings(file_strings, args.input_dir)

    header_filename = args.output_file.format(ext="h")
    source_filename = args.output_file.format(ext="c")

    with open(os.path.join(args.output_dir, header_filename), "w+") as file:
        file.write(generate_header_file(file_strings))
    
    with open(os.path.join(args.output_dir, source_filename), "w+") as file:
        file.write(generate_source_file(file_strings, header_filename))


if __name__ == '__main__':
    main()

