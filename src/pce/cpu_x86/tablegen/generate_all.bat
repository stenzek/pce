SET PATH=%PATH%;"C:\Program Files (x86)\Microsoft Visual Studio\Shared\Python37_64"

python generate_decoder.py x86 ..\decoder_tables.inl
python generate_interpreter_dispatch.py x86 ..\interpreter_dispatch.inl

python generate_decoder.py 8086 ..\..\cpu_8086\decoder_tables.inl
python generate_interpreter_dispatch.py 8086 ..\..\cpu_8086\instructions_dispatch.inl