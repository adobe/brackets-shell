# Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

{
  'includes': [
    # Bring in the autogenerated source file lists.
    'deps/cef/cef_paths.gypi',
   ],
  'variables': {
    'includes_common': [
      'include/base/cef_atomic_ref_count.h',
      'include/base/cef_atomicops.h',
      'include/base/cef_basictypes.h',
      'include/base/cef_bind.h',
      'include/base/cef_bind_helpers.h',
      'include/base/cef_build.h',
      'include/base/cef_callback.h',
      'include/base/cef_callback_forward.h',
      'include/base/cef_callback_helpers.h',
      'include/base/cef_callback_list.h',
      'include/base/cef_cancelable_callback.h',
      'include/base/cef_lock.h',
      'include/base/cef_logging.h',
      'include/base/cef_macros.h',
      'include/base/cef_move.h',
      'include/base/cef_platform_thread.h',
      'include/base/cef_ref_counted.h',
      'include/base/cef_scoped_ptr.h',
      'include/base/cef_string16.h',
      'include/base/cef_template_util.h',
      'include/base/cef_thread_checker.h',
      'include/base/cef_thread_collision_warner.h',
      'include/base/cef_trace_event.h',
      'include/base/cef_tuple.h',
      'include/base/cef_weak_ptr.h',
      'include/base/internal/cef_bind_internal.h',
      'include/base/internal/cef_callback_internal.h',
      'include/base/internal/cef_lock_impl.h',
      'include/base/internal/cef_raw_scoped_refptr_mismatch_checker.h',
      'include/base/internal/cef_thread_checker_impl.h',
      'include/cef_base.h',
      'include/cef_pack_resources.h',
      'include/cef_pack_strings.h',
      'include/cef_runnable.h',
      'include/cef_version.h',
      'include/internal/cef_export.h',
      'include/internal/cef_logging_internal.h',
      'include/internal/cef_ptr.h',
      'include/internal/cef_string.h',
      'include/internal/cef_string_list.h',
      'include/internal/cef_string_map.h',
      'include/internal/cef_string_multimap.h',
      'include/internal/cef_string_types.h',
      'include/internal/cef_string_wrappers.h',
      'include/internal/cef_thread_internal.h',
      'include/internal/cef_time.h',
      'include/internal/cef_trace_event_internal.h',
      'include/internal/cef_types.h',
      'include/internal/cef_types_wrappers.h',
      '<@(autogen_cpp_includes)',
    ],
    'includes_capi': [
      'include/capi/cef_base_capi.h',
      '<@(autogen_capi_includes)',
    ],
    'includes_wrapper': [
      'include/wrapper/cef_byte_read_handler.h',
      'include/wrapper/cef_closure_task.h',
      'include/wrapper/cef_helpers.h',
      'include/wrapper/cef_message_router.h',
      'include/wrapper/cef_resource_manager.h',
      'include/wrapper/cef_stream_resource_handler.h',
      'include/wrapper/cef_xml_object.h',
      'include/wrapper/cef_zip_archive.h',
    ],
    'includes_win': [
      'include/base/internal/cef_atomicops_x86_msvc.h',
      'include/base/internal/cef_bind_internal_win.h',
      'include/cef_sandbox_win.h',
      'include/internal/cef_types_win.h',
      'include/internal/cef_win.h',
    ],
    'includes_mac': [
      'include/base/internal/cef_atomicops_atomicword_compat.h',
      'include/base/internal/cef_atomicops_mac.h',
      'include/cef_application_mac.h',
      'include/internal/cef_mac.h',
      'include/internal/cef_types_mac.h',
    ],
    'includes_linux': [
      'include/base/internal/cef_atomicops_atomicword_compat.h',
      'include/base/internal/cef_atomicops_x86_gcc.h',
      'include/internal/cef_linux.h',
      'include/internal/cef_types_linux.h',
    ],
    'libcef_sources_common': [
      'libcef_dll/cpptoc/cpptoc.h',
      'libcef_dll/ctocpp/base_ctocpp.cc',
      'libcef_dll/ctocpp/base_ctocpp.h',
      'libcef_dll/ctocpp/ctocpp.h',
      'libcef_dll/libcef_dll.cc',
      'libcef_dll/libcef_dll2.cc',
      'libcef_dll/resource.h',
      'libcef_dll/transfer_util.cc',
      'libcef_dll/transfer_util.h',
      'libcef_dll/wrapper_types.h',
      '<@(autogen_library_side)',
    ],
    'libcef_dll_wrapper_sources_common': [
      'libcef_dll/base/cef_atomicops_x86_gcc.cc',
      'libcef_dll/base/cef_bind_helpers.cc',
      'libcef_dll/base/cef_callback_helpers.cc',
      'libcef_dll/base/cef_callback_internal.cc',
      'libcef_dll/base/cef_lock.cc',
      'libcef_dll/base/cef_lock_impl.cc',
      'libcef_dll/base/cef_logging.cc',
      'libcef_dll/base/cef_ref_counted.cc',
      'libcef_dll/base/cef_string16.cc',
      'libcef_dll/base/cef_thread_checker_impl.cc',
      'libcef_dll/base/cef_thread_collision_warner.cc',
      'libcef_dll/base/cef_weak_ptr.cc',
      'libcef_dll/cpptoc/base_cpptoc.cc',
      'libcef_dll/cpptoc/base_cpptoc.h',
      'libcef_dll/cpptoc/cpptoc.h',
      'libcef_dll/ctocpp/ctocpp.h',
      'libcef_dll/transfer_util.cc',
      'libcef_dll/transfer_util.h',
      'libcef_dll/wrapper_types.h',
      'libcef_dll/wrapper/cef_browser_info_map.h',
      'libcef_dll/wrapper/cef_byte_read_handler.cc',
      'libcef_dll/wrapper/cef_closure_task.cc',
      'libcef_dll/wrapper/cef_message_router.cc',
      'libcef_dll/wrapper/cef_resource_manager.cc',
      'libcef_dll/wrapper/cef_stream_resource_handler.cc',
      'libcef_dll/wrapper/cef_xml_object.cc',
      'libcef_dll/wrapper/cef_zip_archive.cc',
      'libcef_dll/wrapper/libcef_dll_wrapper.cc',
      'libcef_dll/wrapper/libcef_dll_wrapper2.cc',
      # '<@(autogen_client_side)',
      # 'autogen_client_side': [
      'libcef_dll/cpptoc/app_cpptoc.cc',
      'libcef_dll/cpptoc/app_cpptoc.h',
      'libcef_dll/ctocpp/auth_callback_ctocpp.cc',
      'libcef_dll/ctocpp/auth_callback_ctocpp.h',
      'libcef_dll/ctocpp/before_download_callback_ctocpp.cc',
      'libcef_dll/ctocpp/before_download_callback_ctocpp.h',
      'libcef_dll/ctocpp/binary_value_ctocpp.cc',
      'libcef_dll/ctocpp/binary_value_ctocpp.h',
      'libcef_dll/ctocpp/browser_ctocpp.cc',
      'libcef_dll/ctocpp/browser_ctocpp.h',
      'libcef_dll/ctocpp/browser_host_ctocpp.cc',
      'libcef_dll/ctocpp/browser_host_ctocpp.h',
      'libcef_dll/cpptoc/browser_process_handler_cpptoc.cc',
      'libcef_dll/cpptoc/browser_process_handler_cpptoc.h',
      'libcef_dll/ctocpp/callback_ctocpp.cc',
      'libcef_dll/ctocpp/callback_ctocpp.h',
      'libcef_dll/cpptoc/client_cpptoc.cc',
      'libcef_dll/cpptoc/client_cpptoc.h',
      'libcef_dll/ctocpp/command_line_ctocpp.cc',
      'libcef_dll/ctocpp/command_line_ctocpp.h',
      'libcef_dll/cpptoc/completion_callback_cpptoc.cc',
      'libcef_dll/cpptoc/completion_callback_cpptoc.h',
      'libcef_dll/cpptoc/context_menu_handler_cpptoc.cc',
      'libcef_dll/cpptoc/context_menu_handler_cpptoc.h',
      'libcef_dll/ctocpp/context_menu_params_ctocpp.cc',
      'libcef_dll/ctocpp/context_menu_params_ctocpp.h',
      'libcef_dll/ctocpp/cookie_manager_ctocpp.cc',
      'libcef_dll/ctocpp/cookie_manager_ctocpp.h',
      'libcef_dll/cpptoc/cookie_visitor_cpptoc.cc',
      'libcef_dll/cpptoc/cookie_visitor_cpptoc.h',
      'libcef_dll/ctocpp/domdocument_ctocpp.cc',
      'libcef_dll/ctocpp/domdocument_ctocpp.h',
      'libcef_dll/ctocpp/domnode_ctocpp.cc',
      'libcef_dll/ctocpp/domnode_ctocpp.h',
      'libcef_dll/cpptoc/domvisitor_cpptoc.cc',
      'libcef_dll/cpptoc/domvisitor_cpptoc.h',
      'libcef_dll/cpptoc/delete_cookies_callback_cpptoc.cc',
      'libcef_dll/cpptoc/delete_cookies_callback_cpptoc.h',
      'libcef_dll/cpptoc/dialog_handler_cpptoc.cc',
      'libcef_dll/cpptoc/dialog_handler_cpptoc.h',
      'libcef_dll/ctocpp/dictionary_value_ctocpp.cc',
      'libcef_dll/ctocpp/dictionary_value_ctocpp.h',
      'libcef_dll/cpptoc/display_handler_cpptoc.cc',
      'libcef_dll/cpptoc/display_handler_cpptoc.h',
      'libcef_dll/cpptoc/download_handler_cpptoc.cc',
      'libcef_dll/cpptoc/download_handler_cpptoc.h',
      'libcef_dll/ctocpp/download_item_ctocpp.cc',
      'libcef_dll/ctocpp/download_item_ctocpp.h',
      'libcef_dll/ctocpp/download_item_callback_ctocpp.cc',
      'libcef_dll/ctocpp/download_item_callback_ctocpp.h',
      'libcef_dll/ctocpp/drag_data_ctocpp.cc',
      'libcef_dll/ctocpp/drag_data_ctocpp.h',
      'libcef_dll/cpptoc/drag_handler_cpptoc.cc',
      'libcef_dll/cpptoc/drag_handler_cpptoc.h',
      'libcef_dll/cpptoc/end_tracing_callback_cpptoc.cc',
      'libcef_dll/cpptoc/end_tracing_callback_cpptoc.h',
      'libcef_dll/ctocpp/file_dialog_callback_ctocpp.cc',
      'libcef_dll/ctocpp/file_dialog_callback_ctocpp.h',
      'libcef_dll/cpptoc/find_handler_cpptoc.cc',
      'libcef_dll/cpptoc/find_handler_cpptoc.h',
      'libcef_dll/cpptoc/focus_handler_cpptoc.cc',
      'libcef_dll/cpptoc/focus_handler_cpptoc.h',
      'libcef_dll/ctocpp/frame_ctocpp.cc',
      'libcef_dll/ctocpp/frame_ctocpp.h',
      'libcef_dll/ctocpp/geolocation_callback_ctocpp.cc',
      'libcef_dll/ctocpp/geolocation_callback_ctocpp.h',
      'libcef_dll/cpptoc/geolocation_handler_cpptoc.cc',
      'libcef_dll/cpptoc/geolocation_handler_cpptoc.h',
      'libcef_dll/cpptoc/get_geolocation_callback_cpptoc.cc',
      'libcef_dll/cpptoc/get_geolocation_callback_cpptoc.h',
      'libcef_dll/ctocpp/jsdialog_callback_ctocpp.cc',
      'libcef_dll/ctocpp/jsdialog_callback_ctocpp.h',
      'libcef_dll/cpptoc/jsdialog_handler_cpptoc.cc',
      'libcef_dll/cpptoc/jsdialog_handler_cpptoc.h',
      'libcef_dll/cpptoc/keyboard_handler_cpptoc.cc',
      'libcef_dll/cpptoc/keyboard_handler_cpptoc.h',
      'libcef_dll/cpptoc/life_span_handler_cpptoc.cc',
      'libcef_dll/cpptoc/life_span_handler_cpptoc.h',
      'libcef_dll/ctocpp/list_value_ctocpp.cc',
      'libcef_dll/ctocpp/list_value_ctocpp.h',
      'libcef_dll/cpptoc/load_handler_cpptoc.cc',
      'libcef_dll/cpptoc/load_handler_cpptoc.h',
      'libcef_dll/ctocpp/menu_model_ctocpp.cc',
      'libcef_dll/ctocpp/menu_model_ctocpp.h',
      'libcef_dll/ctocpp/navigation_entry_ctocpp.cc',
      'libcef_dll/ctocpp/navigation_entry_ctocpp.h',
      'libcef_dll/cpptoc/navigation_entry_visitor_cpptoc.cc',
      'libcef_dll/cpptoc/navigation_entry_visitor_cpptoc.h',
      'libcef_dll/cpptoc/pdf_print_callback_cpptoc.cc',
      'libcef_dll/cpptoc/pdf_print_callback_cpptoc.h',
      'libcef_dll/ctocpp/post_data_ctocpp.cc',
      'libcef_dll/ctocpp/post_data_ctocpp.h',
      'libcef_dll/ctocpp/post_data_element_ctocpp.cc',
      'libcef_dll/ctocpp/post_data_element_ctocpp.h',
      'libcef_dll/ctocpp/print_dialog_callback_ctocpp.cc',
      'libcef_dll/ctocpp/print_dialog_callback_ctocpp.h',
      'libcef_dll/cpptoc/print_handler_cpptoc.cc',
      'libcef_dll/cpptoc/print_handler_cpptoc.h',
      'libcef_dll/ctocpp/print_job_callback_ctocpp.cc',
      'libcef_dll/ctocpp/print_job_callback_ctocpp.h',
      'libcef_dll/ctocpp/print_settings_ctocpp.cc',
      'libcef_dll/ctocpp/print_settings_ctocpp.h',
      'libcef_dll/ctocpp/process_message_ctocpp.cc',
      'libcef_dll/ctocpp/process_message_ctocpp.h',
      'libcef_dll/cpptoc/read_handler_cpptoc.cc',
      'libcef_dll/cpptoc/read_handler_cpptoc.h',
      'libcef_dll/cpptoc/render_handler_cpptoc.cc',
      'libcef_dll/cpptoc/render_handler_cpptoc.h',
      'libcef_dll/cpptoc/render_process_handler_cpptoc.cc',
      'libcef_dll/cpptoc/render_process_handler_cpptoc.h',
      'libcef_dll/ctocpp/request_ctocpp.cc',
      'libcef_dll/ctocpp/request_ctocpp.h',
      'libcef_dll/ctocpp/request_callback_ctocpp.cc',
      'libcef_dll/ctocpp/request_callback_ctocpp.h',
      'libcef_dll/ctocpp/request_context_ctocpp.cc',
      'libcef_dll/ctocpp/request_context_ctocpp.h',
      'libcef_dll/cpptoc/request_context_handler_cpptoc.cc',
      'libcef_dll/cpptoc/request_context_handler_cpptoc.h',
      'libcef_dll/cpptoc/request_handler_cpptoc.cc',
      'libcef_dll/cpptoc/request_handler_cpptoc.h',
      'libcef_dll/cpptoc/resolve_callback_cpptoc.cc',
      'libcef_dll/cpptoc/resolve_callback_cpptoc.h',
      'libcef_dll/ctocpp/resource_bundle_ctocpp.cc',
      'libcef_dll/ctocpp/resource_bundle_ctocpp.h',
      'libcef_dll/cpptoc/resource_bundle_handler_cpptoc.cc',
      'libcef_dll/cpptoc/resource_bundle_handler_cpptoc.h',
      'libcef_dll/cpptoc/resource_handler_cpptoc.cc',
      'libcef_dll/cpptoc/resource_handler_cpptoc.h',
      'libcef_dll/ctocpp/response_ctocpp.cc',
      'libcef_dll/ctocpp/response_ctocpp.h',
      'libcef_dll/cpptoc/response_filter_cpptoc.cc',
      'libcef_dll/cpptoc/response_filter_cpptoc.h',
      'libcef_dll/ctocpp/run_context_menu_callback_ctocpp.cc',
      'libcef_dll/ctocpp/run_context_menu_callback_ctocpp.h',
      'libcef_dll/cpptoc/run_file_dialog_callback_cpptoc.cc',
      'libcef_dll/cpptoc/run_file_dialog_callback_cpptoc.h',
      'libcef_dll/ctocpp/sslcert_principal_ctocpp.cc',
      'libcef_dll/ctocpp/sslcert_principal_ctocpp.h',
      'libcef_dll/ctocpp/sslinfo_ctocpp.cc',
      'libcef_dll/ctocpp/sslinfo_ctocpp.h',
      'libcef_dll/cpptoc/scheme_handler_factory_cpptoc.cc',
      'libcef_dll/cpptoc/scheme_handler_factory_cpptoc.h',
      'libcef_dll/ctocpp/scheme_registrar_ctocpp.cc',
      'libcef_dll/ctocpp/scheme_registrar_ctocpp.h',
      'libcef_dll/cpptoc/set_cookie_callback_cpptoc.cc',
      'libcef_dll/cpptoc/set_cookie_callback_cpptoc.h',
      'libcef_dll/ctocpp/stream_reader_ctocpp.cc',
      'libcef_dll/ctocpp/stream_reader_ctocpp.h',
      'libcef_dll/ctocpp/stream_writer_ctocpp.cc',
      'libcef_dll/ctocpp/stream_writer_ctocpp.h',
      'libcef_dll/cpptoc/string_visitor_cpptoc.cc',
      'libcef_dll/cpptoc/string_visitor_cpptoc.h',
      'libcef_dll/cpptoc/task_cpptoc.cc',
      'libcef_dll/cpptoc/task_cpptoc.h',
      'libcef_dll/ctocpp/task_runner_ctocpp.cc',
      'libcef_dll/ctocpp/task_runner_ctocpp.h',
      # 'libcef_dll/ctocpp/test/translator_test_ctocpp.cc',
      'libcef_dll/ctocpp/test/translator_test_ctocpp.h',
      # 'libcef_dll/cpptoc/test/translator_test_handler_cpptoc.cc',
      'libcef_dll/cpptoc/test/translator_test_handler_cpptoc.h',
      # 'libcef_dll/cpptoc/test/translator_test_handler_child_cpptoc.cc',
      'libcef_dll/cpptoc/test/translator_test_handler_child_cpptoc.h',
      # 'libcef_dll/ctocpp/test/translator_test_object_ctocpp.cc',
      'libcef_dll/ctocpp/test/translator_test_object_ctocpp.h',
      # 'libcef_dll/ctocpp/test/translator_test_object_child_ctocpp.cc',
      'libcef_dll/ctocpp/test/translator_test_object_child_ctocpp.h',
      # 'libcef_dll/ctocpp/test/translator_test_object_child_child_ctocpp.cc',
      'libcef_dll/ctocpp/test/translator_test_object_child_child_ctocpp.h',
      'libcef_dll/ctocpp/urlrequest_ctocpp.cc',
      'libcef_dll/ctocpp/urlrequest_ctocpp.h',
      'libcef_dll/cpptoc/urlrequest_client_cpptoc.cc',
      'libcef_dll/cpptoc/urlrequest_client_cpptoc.h',
      'libcef_dll/cpptoc/v8accessor_cpptoc.cc',
      'libcef_dll/cpptoc/v8accessor_cpptoc.h',
      'libcef_dll/ctocpp/v8context_ctocpp.cc',
      'libcef_dll/ctocpp/v8context_ctocpp.h',
      'libcef_dll/ctocpp/v8exception_ctocpp.cc',
      'libcef_dll/ctocpp/v8exception_ctocpp.h',
      'libcef_dll/cpptoc/v8handler_cpptoc.cc',
      'libcef_dll/cpptoc/v8handler_cpptoc.h',
      'libcef_dll/ctocpp/v8stack_frame_ctocpp.cc',
      'libcef_dll/ctocpp/v8stack_frame_ctocpp.h',
      'libcef_dll/ctocpp/v8stack_trace_ctocpp.cc',
      'libcef_dll/ctocpp/v8stack_trace_ctocpp.h',
      'libcef_dll/ctocpp/v8value_ctocpp.cc',
      'libcef_dll/ctocpp/v8value_ctocpp.h',
      'libcef_dll/ctocpp/value_ctocpp.cc',
      'libcef_dll/ctocpp/value_ctocpp.h',
      'libcef_dll/ctocpp/web_plugin_info_ctocpp.cc',
      'libcef_dll/ctocpp/web_plugin_info_ctocpp.h',
      'libcef_dll/cpptoc/web_plugin_info_visitor_cpptoc.cc',
      'libcef_dll/cpptoc/web_plugin_info_visitor_cpptoc.h',
      'libcef_dll/cpptoc/web_plugin_unstable_callback_cpptoc.cc',
      'libcef_dll/cpptoc/web_plugin_unstable_callback_cpptoc.h',
      'libcef_dll/cpptoc/write_handler_cpptoc.cc',
      'libcef_dll/cpptoc/write_handler_cpptoc.h',
      'libcef_dll/ctocpp/xml_reader_ctocpp.cc',
      'libcef_dll/ctocpp/xml_reader_ctocpp.h',
      'libcef_dll/ctocpp/zip_reader_ctocpp.cc',
      'libcef_dll/ctocpp/zip_reader_ctocpp.h',
    ],
    'appshell_sources_browser': [
      'appshell/browser/binding_test.cc',
      'appshell/browser/binding_test.h',
      'appshell/browser/browser_window.cc',
      'appshell/browser/browser_window.h',
      'appshell/browser/bytes_write_handler.cc',
      'appshell/browser/bytes_write_handler.h',
      'appshell/browser/client_app_browser.cc',
      'appshell/browser/client_app_browser.h',
      'appshell/browser/client_app_delegates_browser.cc',
      'appshell/browser/client_handler.cc',
      'appshell/browser/client_handler.h',
      'appshell/browser/client_handler_std.cc',
      'appshell/browser/client_handler_std.h',
      'appshell/browser/client_types.h',
      'appshell/browser/dialog_test.cc',
      'appshell/browser/dialog_test.h',
      'appshell/browser/geometry_util.cc',
      'appshell/browser/geometry_util.h',
      'appshell/browser/main_context.cc',
      'appshell/browser/main_context.h',
      'appshell/browser/main_context_impl.cc',
      'appshell/browser/main_context_impl.h',
      'appshell/browser/main_message_loop.h',
      'appshell/browser/main_message_loop.cc',
      'appshell/browser/main_message_loop_std.h',
      'appshell/browser/main_message_loop_std.cc',
      'appshell/browser/preferences_test.cc',
      'appshell/browser/preferences_test.h',
      'appshell/browser/resource.h',
      'appshell/browser/resource_util.h',
      'appshell/browser/response_filter_test.cc',
      'appshell/browser/response_filter_test.h',
      'appshell/browser/root_window.cc',
      'appshell/browser/root_window.h',
      'appshell/browser/root_window_manager.cc',
      'appshell/browser/root_window_manager.h',
      'appshell/browser/scheme_test.cc',
      'appshell/browser/scheme_test.h',
      'appshell/browser/temp_window.h',
      'appshell/browser/test_runner.cc',
      'appshell/browser/test_runner.h',
      'appshell/browser/urlrequest_test.cc',
      'appshell/browser/urlrequest_test.h',
      'appshell/browser/window_test.cc',
      'appshell/browser/window_test.h',
    ],
    'appshell_sources_common': [
      'appshell/common/client_app.cc',
      'appshell/common/client_app.h',
      'appshell/common/client_app_delegates_common.cc',
      'appshell/common/client_app_other.cc',
      'appshell/common/client_app_other.h',
      'appshell/common/client_switches.cc',
      'appshell/common/client_switches.h',
      'appshell/common/scheme_test_common.cc',
      'appshell/common/scheme_test_common.h',
      'appshell/appshell_extension_handler.h',
      'appshell/appshell_extensions_platform.h',
      'appshell/appshell_extensions.js',
      'appshell/appshell_helpers.h',
      'appshell/appshell_node_process.h',
      'appshell/appshell_node_process_internal.h',
      'appshell/appshell_node_process.cpp',
      'appshell/command_callbacks.h',
      'appshell/config.h',
      'appshell/native_menu_model.cpp',
      'appshell/native_menu_model.h',
    ],
    'appshell_sources_renderer': [
      'appshell/renderer/client_app_delegates_renderer.cc',
      'appshell/renderer/client_app_renderer.cc',
      'appshell/renderer/client_app_renderer.h',
      'appshell/renderer/client_renderer.cc',
      'appshell/renderer/client_renderer.h',
      'appshell/renderer/performance_test.cc',
      'appshell/renderer/performance_test.h',
      'appshell/renderer/performance_test_setup.h',
      'appshell/renderer/performance_test_tests.cc',
    ],
    'appshell_sources_resources': [
      'appshell/resources/binding.html',
      'appshell/resources/dialogs.html',
      'appshell/resources/localstorage.html',
      'appshell/resources/logo.png',
      'appshell/resources/osr_test.html',
      'appshell/resources/other_tests.html',
      'appshell/resources/pdf.html',
      'appshell/resources/pdf.pdf',
      'appshell/resources/performance.html',
      'appshell/resources/performance2.html',
      'appshell/resources/preferences.html',
      'appshell/resources/response_filter.html',
      'appshell/resources/transparency.html',
      'appshell/resources/urlrequest.html',
      'appshell/resources/window.html',
      'appshell/resources/xmlhttprequest.html',
    ],
    'appshell_sources_win': [
      'appshell/browser/browser_window_osr_win.cc',
      'appshell/browser/browser_window_osr_win.h',
      'appshell/browser/browser_window_std_win.cc',
      'appshell/browser/browser_window_std_win.h',
      'appshell/browser/main_context_impl_win.cc',
      'appshell/browser/main_message_loop_multithreaded_win.cc',
      'appshell/browser/main_message_loop_multithreaded_win.h',
      'appshell/browser/osr_dragdrop_win.cc',
      'appshell/browser/osr_dragdrop_win.h',
      'appshell/browser/osr_window_win.cc',
      'appshell/browser/osr_window_win.h',
      'appshell/browser/resource_util_win.cc',
      'appshell/browser/root_window_win.cc',
      'appshell/browser/root_window_win.h',
      'appshell/browser/temp_window_win.cc',
      'appshell/browser/temp_window_win.h',
      'appshell/browser/util_win.cc',
      'appshell/browser/util_win.h',
      'appshell/browser/window_test_win.cc',
      'appshell/cefclient_win.cc',
      'appshell/resources/win/cefclient.exe.manifest',
      'appshell/resources/win/cefclient.ico',
      'appshell/resources/win/cefclient.rc',
      'appshell/resources/win/small.ico',
      'appshell/browser/client_handler_osr.cc',
      'appshell/browser/client_handler_osr.h',
      'appshell/browser/osr_dragdrop_events.h',
      'appshell/browser/osr_renderer.h',
      'appshell/browser/osr_renderer.cc',
      'appshell/appshell_extensions_win.cpp',
      'appshell/appshell_helpers_win.cc',
      'appshell/appshell_node_process_win.cpp',
      'appshell/appshell_process_message.h',
      '<@(appshell_sources_browser)',
      '<@(appshell_sources_common)',
      '<@(appshell_sources_renderer)',
      '<@(appshell_sources_resources)',
    ],
    'appshell_sources_mac': [
      'appshell/browser/browser_window_osr_mac.h',
      'appshell/browser/browser_window_osr_mac.mm',
      'appshell/browser/browser_window_std_mac.h',
      'appshell/browser/browser_window_std_mac.mm',
      'appshell/browser/main_context_impl_posix.cc',
      'appshell/browser/resource_util_mac.mm',
      'appshell/browser/resource_util_posix.cc',
      'appshell/browser/root_window_mac.h',
      'appshell/browser/root_window_mac.mm',
      'appshell/browser/temp_window_mac.h',
      'appshell/browser/temp_window_mac.mm',
      'appshell/browser/window_test_mac.mm',
      'appshell/cefclient_mac.mm',
      'appshell/browser/client_handler_osr.cc',
      'appshell/browser/client_handler_osr.h',
      'appshell/browser/osr_dragdrop_events.h',
      'appshell/browser/osr_renderer.h',
      'appshell/browser/osr_renderer.cc',
      'appshell/appshell_extensions_mac.mm',
      'appshell/appshell_helpers_mac.mm',
      'appshell/appshell_node_process_mac.mm',
      'appshell/appshell_process_message.h',
      '<@(appshell_sources_browser)',
      '<@(appshell_sources_common)',
   ],
    'appshell_sources_mac_helper': [
      'appshell/process_helper_mac.cc',
      'appshell/appshell_extensions_mac.mm',
      'appshell/appshell_helpers_mac.mm',
      'appshell/appshell_node_process_mac.mm',
      'appshell/appshell_process_message.h',
      '<@(appshell_sources_common)',
      '<@(appshell_sources_renderer)',
    ],
    'appshell_bundle_resources_mac': [
      'appshell/resources/mac/appshell.icns',
      'appshell/resources/mac/English.lproj/InfoPlist.strings',
      'appshell/resources/mac/English.lproj/MainMenu.xib',
      'appshell/resources/mac/French.lproj/InfoPlist.strings',
      'appshell/resources/mac/French.lproj/MainMenu.xib',
      'appshell/resources/mac/Japanese.lproj/InfoPlist.strings',
      'appshell/resources/mac/Japanese.lproj/MainMenu.xib',
      'appshell/resources/mac/Info.plist',
      'appshell/resources/mac/brackets.sh',
      'appshell/resources/mac/TrafficLights.xib',
      'appshell/resources/mac/FullScreen.xib',
      'appshell/resources/mac/window-close-active.png',
      'appshell/resources/mac/window-close-active@2x.png',
      'appshell/resources/mac/window-close-dirty-active.png',
      'appshell/resources/mac/window-close-dirty-active@2x.png',
      'appshell/resources/mac/window-close-dirty-hover.png',
      'appshell/resources/mac/window-close-dirty-hover@2x.png',
      'appshell/resources/mac/window-close-dirty-inactive.png',
      'appshell/resources/mac/window-close-dirty-inactive@2x.png',
      'appshell/resources/mac/window-close-dirty-pressed.png',
      'appshell/resources/mac/window-close-dirty-pressed@2x.png',
      'appshell/resources/mac/window-close-hover.png',
      'appshell/resources/mac/window-close-hover@2x.png',
      'appshell/resources/mac/window-close-inactive.png',
      'appshell/resources/mac/window-close-inactive@2x.png',
      'appshell/resources/mac/window-close-pressed.png',
      'appshell/resources/mac/window-close-pressed@2x.png',
      'appshell/resources/mac/window-minimize-active.png',
      'appshell/resources/mac/window-minimize-active@2x.png',
      'appshell/resources/mac/window-minimize-hover.png',
      'appshell/resources/mac/window-minimize-hover@2x.png',
      'appshell/resources/mac/window-minimize-inactive.png',
      'appshell/resources/mac/window-minimize-inactive@2x.png',
      'appshell/resources/mac/window-minimize-pressed.png',
      'appshell/resources/mac/window-minimize-pressed@2x.png',
      'appshell/resources/mac/window-zoom-active.png',
      'appshell/resources/mac/window-zoom-active@2x.png',
      'appshell/resources/mac/window-zoom-hover.png',
      'appshell/resources/mac/window-zoom-hover@2x.png',
      'appshell/resources/mac/window-zoom-inactive.png',
      'appshell/resources/mac/window-zoom-inactive@2x.png',
      'appshell/resources/mac/window-zoom-pressed.png',
      'appshell/resources/mac/window-zoom-pressed@2x.png',
      'appshell/resources/mac/window-fullscreen-pressed.png',
      'appshell/resources/mac/window-fullscreen-pressed@2x.png',
      'appshell/resources/mac/window-fullscreen-hover.png',
      'appshell/resources/mac/window-fullscreen-hover@2x.png',
      'appshell/resources/mac/window-fullscreen-active.png',
      'appshell/resources/mac/window-fullscreen-active@2x.png',
      'appshell/resources/mac/window-fullscreen-inactive.png',
      'appshell/resources/mac/window-fullscreen-inactive@2x.png',
      'appshell/appshell_extensions.js',
      '<@(appshell_sources_resources)',
    ],
    'appshell_sources_linux': [
      'appshell/browser/browser_window_std_gtk.cc',
      'appshell/browser/browser_window_std_gtk.h',
      'appshell/browser/dialog_handler_gtk.cc',
      'appshell/browser/dialog_handler_gtk.h',
      'appshell/browser/main_context_impl_posix.cc',
      'appshell/browser/print_handler_gtk.cc',
      'appshell/browser/print_handler_gtk.h',
      'appshell/browser/resource_util_linux.cc',
      'appshell/browser/resource_util_posix.cc',
      'appshell/browser/root_window_gtk.cc',
      'appshell/browser/root_window_gtk.h',
      'appshell/browser/temp_window_x11.cc',
      'appshell/browser/temp_window_x11.h',
      'appshell/browser/window_test_gtk.cc',
      'appshell/cefclient_gtk.cc',
      'appshell/appshell_extensions_gtk.cpp',
      'appshell/appshell_helpers_gtk.cc',
      'appshell/appshell_node_process_linux.cpp',
      '<@(appshell_sources_browser)',
      '<@(appshell_sources_common)',
      '<@(appshell_sources_renderer)',
    ],
    'appshell_bundle_resources_linux': [
      '<@(appshell_sources_resources)',
    ],
  },
}
