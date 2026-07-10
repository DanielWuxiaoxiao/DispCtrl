%% Export MATLAB RAE datasets for DispCtrl offline drawing.
% Output files use the RAE1 little-endian binary format consumed by
% Controller/RaeDatasetReader.
%
% Record columns in source tracks follow the plotting scripts:
%   col 3 = range, col 4 = azimuth deg, col 5 = elevation deg
%
% Point type mapping:
%   2 = track. Offline color is stored separately in targetRecResult.
% Offline color mapping:
%   101 = cyan, 102 = red, 103 = blue

clear; clc;

scriptDir = fileparts(mfilename('fullpath'));
oldDir = pwd;
cd(scriptDir);
cleanup = onCleanup(@() cd(oldDir));

outDir = fullfile(scriptDir, 'exported_rae');
if ~exist(outDir, 'dir')
    mkdir(outDir);
end

rangeScaleToM = 1000.0;  % existing scripts label coordinates in km
sampleIntervalMs = 100;

exportAdsbRae(fullfile(outDir, 'adsb_rae.bin'), rangeScaleToM, sampleIntervalMs);
exportCompareRae(fullfile(outDir, 'compare_rae.bin'), rangeScaleToM, sampleIntervalMs);

fprintf('Export complete:\n  %s\n  %s\n', ...
    fullfile(outDir, 'adsb_rae.bin'), ...
    fullfile(outDir, 'compare_rae.bin'));

function exportAdsbRae(outFile, rangeScaleToM, sampleIntervalMs)
    load('AllTrackCelled.mat');
    load('RadialTrackIdx1.mat');

    unusefulTrackID = [2 5 9 10 11 12 15 20 23 25 26 27 31 32 33 35];
    RadialTrackIdx(unusefulTrackID) = [];

    records = {};

    % adsb.m: all ADS-B tracks, cyan
    for ii = 1:length(TrackADSB)
        a = TrackADSB{ii};
        records = appendTrajectory(records, a, uint32(ii), 101, rangeScaleToM, sampleIntervalMs);
    end

    % adsb.m: radial tracks drawn again as a highlighted layer, red
    for ii = 1:length(TrackADSB)
        if isempty(find(RadialTrackIdx == ii, 1))
            continue;
        end
        a = TrackADSB{ii};
        records = appendTrajectory(records, a, uint32(100000 + ii), 102, rangeScaleToM, sampleIntervalMs);
    end

    writeRae1(outFile, records);
end

function exportCompareRae(outFile, rangeScaleToM, sampleIntervalMs)
    load('AllTrackCelled.mat');
    clear TrackTBD
    load('TrackTBD1.mat');
    load('RadialTrackIdx1.mat');
    load('NewResult.mat');

    unusefulTrackID = [2 5 9 10 11 12 15 20 23 25 26 27 31 32 33 35];
    RadialTrackIdx(unusefulTrackID) = [];
    trackList = RadialTrackIdx;

    targetAdsbId = 7868555;  % same target selected by compare.m
    minLength = 10;
    records = {};

    for jj = 1:length(trackList)
        trackID = trackList(jj);
        a = TrackADSB{trackID};
        if a(1, 1) ~= targetAdsbId
            continue;
        end

        records = appendTrajectory(records, a, uint32(trackID), 101, rangeScaleToM, sampleIntervalMs);

        trackNormalID = NewResult{trackID, 1};
        trackTBDID = NewResult{trackID, 2};

        for ii = 1:length(trackNormalID)
            normalTrack = TrackNormal{1, trackNormalID(ii)};
            if size(normalTrack, 1) < minLength
                continue;
            end
            records = appendTrajectory(records, normalTrack, uint32(200000 + trackNormalID(ii)), 103, rangeScaleToM, sampleIntervalMs);
        end

        for ii = 1:length(trackTBDID)
            tbdTrack = TrackTBD{1, trackTBDID(ii)};
            if size(tbdTrack, 1) < minLength
                continue;
            end
            records = appendTrajectory(records, tbdTrack, uint32(300000 + trackTBDID(ii)), 102, rangeScaleToM, sampleIntervalMs);
        end
    end

    writeRae1(outFile, records);
end

function records = appendTrajectory(records, a, batch, colorCode, rangeScaleToM, sampleIntervalMs)
    if isempty(a) || size(a, 2) < 5
        return;
    end

    for row = 1:size(a, 1)
        rec.timestampMs = uint32((row - 1) * sampleIntervalMs);
        rec.pointType = uint8(2);
        rec.batch = uint32(batch);
        rec.rangeM = single(a(row, 3) * rangeScaleToM);
        rec.azimuthDeg = single(mod(a(row, 4), 360));
        rec.elevationDeg = single(a(row, 5));
        rec.snr = single(0);
        rec.speed = single(0);
        rec.amp = single(0);
        rec.targetRecResult = uint8(colorCode);
        if size(a, 2) >= 6
            rec.speed = single(a(row, 6));
        end
        records{end + 1} = rec; %#ok<AGROW>
    end
end

function writeRae1(outFile, records)
    fid = fopen(outFile, 'w', 'ieee-le');
    if fid < 0
        error('Cannot create %s', outFile);
    end
    cleanup = onCleanup(@() fclose(fid));

    fwrite(fid, uint32(hex2dec('31454152')), 'uint32'); % "RAE1"
    fwrite(fid, uint32(1), 'uint32');
    fwrite(fid, uint32(numel(records)), 'uint32');

    for ii = 1:numel(records)
        rec = records{ii};
        fwrite(fid, rec.timestampMs, 'uint32');
        fwrite(fid, rec.pointType, 'uint8');
        fwrite(fid, uint8([0 0 0]), 'uint8');
        fwrite(fid, rec.batch, 'uint32');
        fwrite(fid, rec.rangeM, 'single');
        fwrite(fid, rec.azimuthDeg, 'single');
        fwrite(fid, rec.elevationDeg, 'single');
        fwrite(fid, rec.snr, 'single');
        fwrite(fid, rec.speed, 'single');
        fwrite(fid, rec.amp, 'single');
        fwrite(fid, rec.targetRecResult, 'uint8');
        fwrite(fid, uint8([0 0 0]), 'uint8');
    end

    fprintf('Wrote %s (%d records)\n', outFile, numel(records));
end
