
import sys, os, re, subprocess, select, plistlib, frida

def print_usage():
	print('python3 install_ios_app.py app_ids')

argv = sys.argv;
argc = len(sys.argv)

if argc < 2:
	print_usage();

env = os.environ.copy()
env['IPATOOL_EMAIL'] = 'ilhan.raja@icloud.com'
env['IPATOOL_2FA_CODE'] = '452633'

command = ['ipatool','auth', 'login']

process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None, env=env)

output, error = process.communicate()

output = output.decode('utf-8')

stream = output.split('\n')

for i in range(1, argc):
	app_id = argv[i]

	command = ['ipatool','search', app_id]

	process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None, env=env)

	output, error = process.communicate()

	output = output.decode('utf-8')

	stream = output.split('\n')

	bundle_id = None

	print('Installing app with app id %s' %(app_id))

	for line in stream:
		regex = re.compile(r'\b(?![A-Z])[\w-]+(?:\.[\w-]+)+\b')

		result = regex.findall(line)

		if result:
			result = result[0].strip().split(' ')

			if result is not None and len(result) != 0:
				bundle_id  = result[0]
			else:
				continue

	if bundle_id is None:
		print_usage()

	print(bundle_id)

	command = ['ipatool','purchase', '--bundle-identifier', bundle_id, '--device-family', 'iPhone']

	process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None, env=env)

	output, error = process.communicate()

	output = output.decode('utf-8')

	stream = output.split('\n')

	command = ['ipatool','download', '--bundle-identifier', bundle_id, '--device-family', 'iPhone']

	process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None, env=env)

	output, error = process.communicate()

	output = output.decode('utf-8')

	stream = output.split('\n')

	ipa_file = None

	for line in stream:

		if bundle_id in line and '.ipa' in line:

			ipa_file = line[line.find(bundle_id):line.find('.ipa') + len('.ipa')]

	if ipa_file is None:
		print_usage()

	print('ipa = %s' % (ipa_file))

	command = ['ideviceinstaller','-i', ipa_file]

	process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)

	output, error = process.communicate()

	output = output.decode('utf-8')

	stream = output.split('\n')

	if error is not None:
		exit(-1)

	num_failures = 0

	complete = False

	while((not complete or 'FATAL' in error or 'ERROR' in error) and num_failures < 10):

		command = ['bagbak','--no-extension', '--override', bundle_id]

		process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

		stdout = []
		stderr = []

		while True:
			reads = [process.stdout.fileno(), process.stderr.fileno()]
			ret = select.select(reads, [], [])

			for fd in ret[0]:

				if fd == process.stdout.fileno():
					read = process.stdout.readline()
					sys.stdout.write(read.decode("utf-8") )
					stdout.append(read.decode("utf-8") )
				
				if fd == process.stderr.fileno():
					read = process.stderr.readline()
					sys.stderr.write(read.decode("utf-8") )
					stderr.append(read.decode("utf-8") )

			if process.poll() != None:
				break

		process.wait()

		output, error = process.communicate()

		output = "".join(stdout)

		error = "".join(stderr)

		stream = output.split('\n')

		print(output)
		print(error)

		complete = True

		num_failures += 1

	if(num_failures >= 10):
		continue

	decrypt_folder = './dump/' + bundle_id

	payload_folder = decrypt_folder + '/Payload'

	command = ['ls', '-1', payload_folder]

	process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)

	output, error = process.communicate()

	output = output.decode('utf-8')

	stream = output.split('\n')

	localized_name = None

	for line in stream:
		if '.app' in line:
			localized_name = line[:line.find('.app')]

			break

	if localized_name is None:
		print_usage()

	app_bundle = payload_folder + '/' + localized_name + '.app'
	app_binary = app_bundle + '/' + localized_name
	app_frameworks = app_bundle + '/' + 'Frameworks'

	command = ['codesign', '-fs', '-', '--preserve-metadata=entitlements', app_binary, '--deep']

	process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)

	output, error = process.communicate()

	output = output.decode('utf-8')

	command = ['ls', '-1', app_frameworks]

	process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)

	output, error = process.communicate()

	output = output.decode('utf-8')

	stream = output.split('\n')

	for line in stream:
		to_sign = app_frameworks + '/' + line

		command = ['codesign', '-fs', '-', '--preserve-metadata=entitlements', to_sign, '--deep']

		process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)

		output, error = process.communicate()

		output = output.decode('utf-8')

	new_ipa = decrypt_folder + '/' + localized_name + '.ipa'

	command = ['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent',  payload_folder, new_ipa]

	process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)

	output, error = process.communicate()

	output = output.decode('utf-8')
