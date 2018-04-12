wav = 'island.wav';
bin = 'island.bin';
MAX_SAMPLES = 4e3;

[x, fs] = audioread(wav);

stop = length(x);
if stop > MAX_SAMPLES
    stop = MAX_SAMPLES;
    disp(sprintf('Chopping to %f thousand samples.', MAX_SAMPLES/1e3));
end

f = fopen(bin, 'w');
fwrite(f, x(1:stop), 'single');
fclose(f);

disp(sprintf('Read from "%s", wrote to "%s".', wav, bin));
disp(sprintf('Sampling rate is %fkHz.', fs/1e3));
