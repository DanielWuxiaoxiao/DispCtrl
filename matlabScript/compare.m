%%绘制7872955航迹
clear all
close all
clc
load('AllTrackCelled.mat');
clear TrackTBD
load('TrackTBD1.mat');
load('RadialTrackIdx1.mat');
load('NewResult.mat');
UnusefulTrackID = [2 5 9 10 11 12 15 20 23 25 26 27 31 32 33 35];
RadialTrackIdx(UnusefulTrackID) = [];
Track = RadialTrackIdx;
% adsb_id = [7872955 7865902];

figure
% PlotCircle(0, 0, 0, 400, 50, 0, 360, 15, 'km', 350, 295); %all track
% PlotCircle(0, 0, 0, 400, 50, 0, 360, 15, 'km', 200, 263); %7872955
PlotCircle(0, 0, 0, 400, 50, 0, 360, 15, 'km', 150, 263); %7868555
% PlotCircle(0, 0, 0, 400, 50, 0, 360, 15, 'km', 200, 275); %7865117
hold on
axis equal;
xlim([-370 -50]), ylim([-150 155]);
set(gcf, 'Position', [100,100,800,600]);
minLength = 10;

for jj = 1:1:length(Track)
    TrackID = Track(jj);
    a = TrackADSB{TrackID};
    
    
    if a(1,1) ~= 7868555
        continue;
    end

    y = a(:,3).*cosd(a(:,5)).*cosd(a(:,4));
    x = a(:,3).*cosd(a(:,5)).*sind(a(:,4));
    h1 = [];
    h1 = plot(x,y,'c','LineWidth',1.5);
    text(x(1), y(1), num2str(a(1,1)));

    TrackNormalID = NewResult{TrackID,1};
    TrackTBDID = NewResult{TrackID,2};

    h3 = [];
    for ii = 1:1:length(TrackTBDID)
        a = TrackTBD{1,TrackTBDID(ii)};
        if size(a,1)<minLength
            continue;
        end
        y = a(:,3).*cosd(a(:,5)).*cosd(a(:,4));
        x = a(:,3).*cosd(a(:,5)).*sind(a(:,4));
        if isempty(h3)
            h3 = plot(x,y,'ro-','LineWidth',0.5, 'MarkerSize',6);
        else
            plot(x,y,'ro-','LineWidth',0.5, 'MarkerSize',6);
        end
    end

    h2 = [];
    for ii = 1:1:length(TrackNormalID)
        a = TrackNormal{1,TrackNormalID(ii)};
        if size(a,1)<minLength
            continue;
        end
        y = a(:,3).*cosd(a(:,5)).*cosd(a(:,4));
        x = a(:,3).*cosd(a(:,5)).*sind(a(:,4));
        if isempty(h2)
            h2 = plot(x,y,'b*-','LineWidth',0.5, 'MarkerSize',6);
        else
            plot(x,y,'b*-','LineWidth',0.5, 'MarkerSize',6);
        end
    end
    legend([h1,h2,h3],{'ADS-B航迹','常规处理航迹','增程探测处理航迹'},...
        'Location','best');
    xlabel('X km','FontSize',20,'FontName','Times New Roman');
    ylabel('Y km','FontSize',20,'FontName','Times New Roman');

    set(gca,...
        'FontSize',18,...
        'LineWidth',1.2);

end
